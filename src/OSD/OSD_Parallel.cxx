// Created on: 2014-08-19
// Created by: Alexander Zaikin
// Copyright (c) 1996-1999 Matra Datavision
// Copyright (c) 2013-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <OSD_Parallel.hxx>

#include <OSD_DirectoryIterator.hxx>

#ifdef _WIN32
  #include <windows.h>
  #include <process.h>
#else
  #include <sys/types.h>
  #include <unistd.h>

  #ifdef __sun
    #include <sys/processor.h>
    #include <sys/procset.h>
  #else
    #include <sched.h>
  #endif
#endif

#include <array>
#include <bitset>
#include <limits>
#include <map>
#include <vector>

#include <Standard_WarningDisableFunctionCast.hxx>

namespace {

#if defined(_WIN32) && !defined(OCCT_UWP)
  //! For a 64-bit app running under 64-bit Windows, this is FALSE.
  static bool isWow64()
  {
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE , PBOOL);
    BOOL bIsWow64 = FALSE;

    HMODULE aKern32Module = GetModuleHandleW(L"kernel32");
    LPFN_ISWOW64PROCESS aFunIsWow64 = (aKern32Module == NULL) ? (LPFN_ISWOW64PROCESS )NULL
      : (LPFN_ISWOW64PROCESS)GetProcAddress(aKern32Module, "IsWow64Process");

    return aFunIsWow64 != NULL &&
           aFunIsWow64(GetCurrentProcess(), &bIsWow64) &&
           bIsWow64 != FALSE;
  }

  //! Local copy of the fixed-layout prefix of SYSTEM_CPU_SET_INFORMATION (winnt.h)
  //! as it is declared only for newer Windows SDK / _WIN32_WINNT.
  struct OcctCpuSetEntry
  {
    DWORD Size;
    DWORD Type;
    DWORD Id;
    WORD Group;
    BYTE LogicalProcessorIndex;
    BYTE CoreIndex;
    BYTE LastLevelCacheIndex;
    BYTE NumaNodeIndex;
    BYTE EfficiencyClass;
    BYTE AllFlags;
    BYTE SchedulingClass;
  };

  //! Computes a group 0 processor affinity mask keeping a single logical CPU per distinct performance core
  //! (Hyper-thread siblings and, on hybrid CPUs, efficiency cores are skipped).
  //!
  //! Selection rules, in priority order:
  //! 1. if EfficiencyClass varies, keep the cores with the maximum EfficiencyClass;
  //! 2. else if SchedulingClass varies, keep the cores above the minimum
  //!    (Intel hybrid reports a uniform EfficiencyClass but separates P/E cores by SchedulingClass);
  //! 3. else the CPU is homogeneous - keep every distinct physical core.
  //!
  //! Returns the number of detected performant cores.
  static int getPerformantCoresMask(DWORD_PTR& theMask)
  {
    typedef BOOL (WINAPI *GetSystemCpuSetInformation_t)(void*, ULONG, ULONG*, HANDLE, ULONG);
    HMODULE aKern32 = GetModuleHandleW(L"kernel32");
    if (aKern32 == nullptr)
      return 0;

    // the API is available since Windows 10; older systems have no such export
    GetSystemCpuSetInformation_t aGetCpuSetInfo = (GetSystemCpuSetInformation_t )GetProcAddress(aKern32, "GetSystemCpuSetInformation");
    if (aGetCpuSetInfo == nullptr)
      return 0;

    const HANDLE aProcess = GetCurrentProcess();
    ULONG aBufLen = 0;
    aGetCpuSetInfo(nullptr, 0, &aBufLen, aProcess, 0);
    if (aBufLen == 0)
      return 0;

    std::vector<unsigned char> aBuffer(aBufLen);
    if (!aGetCpuSetInfo(aBuffer.data(), aBufLen, &aBufLen, aProcess, 0))
      return 0;

    constexpr int aMaskBits = std::numeric_limits<int>::digits;

    // first pass: check whether EfficiencyClass / SchedulingClass vary across CPU sets
    int aMaxEff = -1;
    int aMinEff = IntegerLast();
    int aMaxSched = -1;
    int aMinSched = IntegerLast();
    for (ULONG anOffset = 0; anOffset + sizeof(OcctCpuSetEntry) <= aBufLen;)
    {
      const OcctCpuSetEntry* anEntry = (const OcctCpuSetEntry* )(aBuffer.data() + anOffset);
      if (anEntry->Size < sizeof(OcctCpuSetEntry) || anOffset + anEntry->Size > aBufLen)
        break;

      const int anEff  = (int )anEntry->EfficiencyClass;
      const int aSched = (int )anEntry->SchedulingClass;
      aMaxEff = Max(aMaxEff, anEff);
      aMinEff = Min(aMinEff, anEff);
      aMaxSched = Max(aMaxSched, aSched);
      aMinSched = Min(aMinSched, aSched);
      anOffset += anEntry->Size;
    }

    const bool toUseEff = (aMaxEff > aMinEff);
    const bool toUseSched = !toUseEff && (aMaxSched > aMinSched);

    // second pass: keep the lowest logical CPU index of every performance core
    std::array<int, 256> aMinLpiPerCore;
    for (int& aCoreMinLpi : aMinLpiPerCore)
      aCoreMinLpi = -1;

    int aNbPerf1 = 0;
    for (ULONG anOffset = 0; anOffset + sizeof(OcctCpuSetEntry) <= aBufLen;)
    {
      const OcctCpuSetEntry* anEntry = (const OcctCpuSetEntry* )(aBuffer.data() + anOffset);
      if (anEntry->Size < sizeof(OcctCpuSetEntry) || anOffset + anEntry->Size > aBufLen)
        break;

      const int aGroup = (int )anEntry->Group;
      const int aLpi   = (int )anEntry->LogicalProcessorIndex;
      const int aCore  = (int )anEntry->CoreIndex;
      const int anEff  = (int )anEntry->EfficiencyClass;
      const int aSched = (int )anEntry->SchedulingClass;
      anOffset += anEntry->Size;

      // the mask is a single machine word applied to the process's group 0
      if (aGroup != 0 || aLpi >= aMaskBits)
        continue;

      bool isPerf = true;
      if (toUseEff)
        isPerf = (anEff == aMaxEff);
      else if (toUseSched)
        isPerf = (aSched > aMinSched);

      if (!isPerf)
        continue;

      ++aNbPerf1;
      if (aMinLpiPerCore[aCore] < 0 || aLpi < aMinLpiPerCore[aCore])
        aMinLpiPerCore[aCore] = aLpi;
    }

    theMask = 0;
    int aNbPerf = 0;
    for (size_t aCoreIter = 0; aCoreIter < aMinLpiPerCore.size(); ++aCoreIter)
    {
      if (aMinLpiPerCore[aCoreIter] >= 0)
      {
        theMask |= ((DWORD_PTR )1 << aMinLpiPerCore[aCoreIter]);
        ++aNbPerf;
      }
    }

    return aNbPerf;
  }

#elif defined(__ANDROID__)

  //! Simple number parser.
  static const char* parseNumber (int&        theResult,
                                  const char* theInput,
                                  const char* theLimit,
                                  const int   theBase = 10)
  {
    const char* aCharIter = theInput;
    int aValue = 0;
    while (aCharIter < theLimit)
    {
      int aDigit = (*aCharIter - '0');
      if ((unsigned int )aDigit >= 10U)
      {
        aDigit = (*aCharIter - 'a');
        if ((unsigned int )aDigit >= 6U)
        {
          aDigit = (*aCharIter - 'A');
        }
        if ((unsigned int )aDigit >= 6U)
        {
          break;
        }
        aDigit += 10;
      }
      if (aDigit >= theBase)
      {
        break;
      }
      aValue = aValue * theBase + aDigit;
      ++aCharIter;
    }
    if (aCharIter == theInput)
    {
      return NULL;
    }

    theResult = aValue;
    return aCharIter;
  }

  //! Read CPUs mask from sysfs.
  static uint32_t readCpuMask (const char* thePath)
  {
    FILE* aFileHandle = fopen (thePath, "rb");
    if (aFileHandle == NULL)
    {
      return 0;
    }

    fseek (aFileHandle, 0, SEEK_END);
    long aFileLen = ftell (aFileHandle);
    if (aFileLen <= 0L)
    {
      fclose (aFileHandle);
      return 0;
    }

    char* aBuffer = (char* )Standard::Allocate (aFileLen);
    if (aBuffer == NULL)
    {
      return 0;
    }

    fseek (aFileHandle, 0, SEEK_SET);
    size_t aCountRead = fread (aBuffer, 1, aFileLen, aFileHandle);
    (void )aCountRead;
    fclose (aFileHandle);

    uint32_t aCpuMask = 0;
    const char* anEnd = aBuffer + aFileLen;
    for (const char* aCharIter = aBuffer; aCharIter < anEnd && *aCharIter != '\n';)
    {
      const char* aChunkEnd = (const char* )::memchr (aCharIter, ',', anEnd - aCharIter);
      if (aChunkEnd == NULL)
      {
        aChunkEnd = anEnd;
      }

      // get first value
      int anIndexLower = 0;
      aCharIter = parseNumber (anIndexLower, aCharIter, aChunkEnd);
      if (aCharIter == NULL)
      {
        Standard::Free (aBuffer);
        return aCpuMask;
      }

      // if we're not at the end of the item, expect a dash and integer; extract end value.
      int anIndexUpper = anIndexLower;
      if (aCharIter < aChunkEnd && *aCharIter == '-')
      {
        aCharIter = parseNumber (anIndexUpper, aCharIter + 1, aChunkEnd);
        if (aCharIter == NULL)
        {
          Standard::Free (aBuffer);
          return aCpuMask;
        }
      }

      // set bits CPU list
      for (int aCpuIndex = anIndexLower; aCpuIndex <= anIndexUpper; ++aCpuIndex)
      {
        if ((unsigned int )aCpuIndex < 32)
        {
          aCpuMask |= (uint32_t )(1U << aCpuIndex);
        }
      }

      aCharIter = aChunkEnd;
      if (aCharIter < anEnd)
      {
        ++aCharIter;
      }
    }

    Standard::Free (aBuffer);
    return aCpuMask;
  }
#endif

#if defined(__linux__) && defined(__GLIBC__)
  //! Fills @p theCpuSet with one logical CPU per distinct performance core:
  //! online CPUs are grouped by physical core (physical_package_id:core_id).
  //!
  //! Only the cores running at the global maximum frequency (cpufreq/cpuinfo_max_freq) are kept,
  //! so efficiency cores of hybrid CPUs are dropped - and the lowest-indexed sibling of each surviving core is selected.
  //! When frequency data is unavailable every core is kept (siblings are still deduplicated).
  //!
  //! Returns the number of detected performant cores.
  static int getPerformantCoresMask(cpu_set_t& theCpuSet)
  {
    static constexpr char THE_SYSTEM_CPU_DIR[] = "/sys/devices/system/cpu";

    // read an integer from a file
    const auto readIntFromFile = [](int& theValue, const TCollection_AsciiString& thePath) -> bool
    {
      std::ifstream aFile;
      aFile.open(thePath.ToCString(), std::ios_base::in | std::ios_base::binary);
      if (!aFile.is_open())
        return false;

      char aBuff[64] = {};
      aFile.getline(aBuff, 64);
      if (aBuff[0] == '\0')
        return false;

      theValue = atoi(aBuff);
      return true;
    };

    // (physical_package_id, core_id) -> (max frequency, lowest logical CPU index)
    typedef std::pair<int, int> PackageAndCore;
    struct CoreInfo
    {
      int MaxFreq = 0;
      int CpuId = 0;
    };
    std::map<PackageAndCore, CoreInfo> aMapOfCores;
    int aMaxFreq = 0;

    const TCollection_AsciiString aCpuRootPath(THE_SYSTEM_CPU_DIR);
    const OSD_Path aCpuRootPathOsd(aCpuRootPath);
    for (OSD_DirectoryIterator aCpuIter(aCpuRootPathOsd, "*"); aCpuIter.More(); aCpuIter.Next())
    {
      OSD_Path aCpuPathOsd;
      aCpuIter.Values().Path(aCpuPathOsd);

      TCollection_AsciiString aCpuName;
      aCpuPathOsd.SystemName(aCpuName);
      if (aCpuName.Length() < 4 || !aCpuName.StartsWith("cpu"))
        continue;

      const TCollection_AsciiString aCpuIdStr = aCpuName.SubString(4, aCpuName.Length());
      if (!aCpuIdStr.IsIntegerValue())
        continue;

      const int aCpuId = aCpuIdStr.IntegerValue();
      if (aCpuId < 0 || aCpuId >= CPU_SETSIZE)
        continue;

      int isOnline  = 1; // the "online" node is absent for cpu0 on many systems - treat it as online
      int aPkgId    = 0;
      int aCoreId   = aCpuId;
      int aCoreFreq = 0;

      readIntFromFile(isOnline,  aCpuRootPath + "/" + aCpuName + "/online");
      if (isOnline != 1)
        continue;

      readIntFromFile(aPkgId,    aCpuRootPath + "/" + aCpuName + "/topology/physical_package_id");
      readIntFromFile(aCoreId,   aCpuRootPath + "/" + aCpuName + "/topology/core_id");
      readIntFromFile(aCoreFreq, aCpuRootPath + "/" + aCpuName + "/cpufreq/cpuinfo_max_freq");
      aMaxFreq = std::max(aMaxFreq, aCoreFreq);

      const PackageAndCore aKey(aPkgId, aCoreId);
      auto aCoreIter = aMapOfCores.find(aKey);
      if (aCoreIter == aMapOfCores.end())
      {
        CoreInfo anInfo;
        anInfo.CpuId = aCpuId;
        anInfo.MaxFreq = aCoreFreq;
        aMapOfCores[aKey] = anInfo;
      }
      else
      {
        if (aCoreFreq > aCoreIter->second.MaxFreq)
          aCoreIter->second.MaxFreq = aCoreFreq;

        if ((int )aCpuId < aCoreIter->second.CpuId)
          aCoreIter->second.CpuId = (int )aCpuId;
      }
    }

    if (aMapOfCores.empty())
      return 0;

    int aNbPerf = 0;
    for (const std::pair<const PackageAndCore, CoreInfo>& aCoreIter : aMapOfCores)
    {
      if (aCoreIter.second.MaxFreq == aMaxFreq)
      {
        CPU_SET(aCoreIter.second.CpuId, &theCpuSet);
        ++aNbPerf;
      }
    }
    return aNbPerf;
  }
#endif

  static Standard_Boolean OSD_Parallel_ToUseOcctThreads =
  #ifdef HAVE_TBB
    Standard_False;
  #else
    Standard_True;
  #endif
}

//=======================================================================
//function : ToUseOcctThreads
//purpose  :
//=======================================================================
Standard_Boolean OSD_Parallel::ToUseOcctThreads()
{
  return OSD_Parallel_ToUseOcctThreads;
}

//=======================================================================
//function : SetUseOcctThreads
//purpose  :
//=======================================================================
void OSD_Parallel::SetUseOcctThreads (Standard_Boolean theToUseOcct)
{
#ifdef HAVE_TBB
  OSD_Parallel_ToUseOcctThreads = theToUseOcct;
#else
  (void )theToUseOcct;
#endif
}

//=======================================================================
//function : NbLogicalProcessors
//purpose  :
//=======================================================================
int OSD_Parallel::NbLogicalProcessors()
{
  const auto getNbPinned = []() -> int
  {
    int aNbOnline = 0;
    int aNbPinned = 0;
    NbLogicalProcessors(aNbOnline, &aNbPinned, nullptr);
    return aNbPinned != 0 ? aNbPinned : aNbOnline;
  };

  const int aNbProcessors = getNbPinned();
  return aNbProcessors;
}

//=======================================================================
//function : NbLogicalProcessors
//purpose  :
//=======================================================================
void OSD_Parallel::NbLogicalProcessors(int& theOnline,
                                       int* thePinned,
                                       int* thePerf)
{
  if (thePinned != nullptr)
    *thePinned = 0;

  if (thePerf != nullptr)
    *thePerf = 0;

#ifdef _WIN32
  // GetSystemInfo() will return the number of processors in a data field in a SYSTEM_INFO structure.
  SYSTEM_INFO aSysInfo = {};
#ifndef OCCT_UWP
  // 32-bit Windows is limited to 32 cores, but 32-bit app could run more within WoW64
  typedef BOOL (WINAPI *LPFN_GSI)(LPSYSTEM_INFO );

  const auto getProcGSI = []() -> LPFN_GSI
  {
    HMODULE aKern32 = GetModuleHandleW(L"kernel32");
    return (LPFN_GSI )GetProcAddress(aKern32, "GetNativeSystemInfo");
  };

  static const LPFN_GSI aFuncSysInfo = isWow64() ? getProcGSI() : nullptr;
  if (aFuncSysInfo != nullptr)
    aFuncSysInfo(&aSysInfo);
  else
    GetSystemInfo(&aSysInfo);
#else
  GetNativeSystemInfo(&aSysInfo);
#endif
  theOnline = aSysInfo.dwNumberOfProcessors;

  // Fetch CPU affinity mask for systems having less than 64 cores:
  // - Win7+ support more than 64 cores, split into 'groups',
  //   process is assigned to one group by default (more groups could be exposed via dedicated API);
  // - Win11+ changed behavior so that process may utilize more than 64 cores by default,
  //   with old 'groups' API mixed up with new 'SelectedCpuSetMasks' API.
  // Affinity mask is ignored if it is full or empty.
  if (thePinned != nullptr)
  {
    DWORD_PTR aProcAff = 0, aSysAff = 0;
    const HANDLE aProcess = GetCurrentProcess();
    if (GetProcessAffinityMask(aProcess, &aProcAff, &aSysAff))
    {
      constexpr int aMaxAff = std::numeric_limits<DWORD_PTR>::digits;
      const     int aNbAff  = (int )std::bitset<aMaxAff>(aProcAff).count();
      if (aNbAff > 0 && aNbAff != aMaxAff)
        *thePinned = aNbAff;
    }
  }

  if (thePerf != nullptr)
  {
    DWORD_PTR aMask = 0;
    *thePerf = getPerformantCoresMask(aMask);
  }
#else

#if defined(__ANDROID__)
  uint32_t aCpuMaskPresent  = readCpuMask("/sys/devices/system/cpu/present");
  uint32_t aCpuMaskPossible = readCpuMask("/sys/devices/system/cpu/possible");
  aCpuMaskPresent &= aCpuMaskPossible;
  const int aNbPresent = __builtin_popcount(aCpuMaskPresent);
  if (aNbPresent >= 1)
  {
    theOnline = aNbPresent;
    return;
  }
#endif

  // These are the choices. We'll check number of processors online.
  // _SC_NPROCESSORS_CONF   Number of processors configured
  // _SC_NPROCESSORS_MAX    Max number of processors supported by platform
  // _SC_NPROCESSORS_ONLN   Number of processors online
  long aNbOnline = sysconf(_SC_NPROCESSORS_ONLN);
  theOnline = int(aNbOnline);

#if defined(__linux__) && defined(__GLIBC__)
  cpu_set_t aCpuSet;
  CPU_ZERO(&aCpuSet);
  if (thePinned != nullptr && sched_getaffinity(getpid(), sizeof(aCpuSet), &aCpuSet) == 0)
  {
    int aNbAff = 0;
    for (long aCoreIter = 0; aCoreIter < aNbOnline; ++aCoreIter)
    {
      if (CPU_ISSET(aCoreIter, &aCpuSet))
        ++aNbAff;
    }
    if (aNbAff != 0)
      *thePinned = aNbAff;
  }

  if (thePerf != nullptr)
  {
    CPU_ZERO(&aCpuSet);
    *thePerf = getPerformantCoresMask(aCpuSet);
  }
#endif

#endif
}

//=======================================================================
//function : SetAffinityToPerformantCores
//purpose  :
//=======================================================================
bool OSD_Parallel::SetAffinityToPerformantCores(bool theToSet)
{
#if defined(_WIN32) && !defined(OCCT_UWP)
  (void)theToSet;
  DWORD_PTR aMask = 0;
  if (!theToSet)
  {
    DWORD_PTR aProcAff = 0;
    const HANDLE aProcess = GetCurrentProcess();
    GetProcessAffinityMask(aProcess, &aProcAff, &aMask);
  }
  else if (getPerformantCoresMask(aMask) == 0)
  {
    return false;
  }

  if (SetProcessAffinityMask(GetCurrentProcess(), aMask))
    return true;
#elif defined(__linux__) && defined(__GLIBC__)
  cpu_set_t aCpuSet;
  CPU_ZERO(&aCpuSet);
  if (!theToSet)
  {
    long aNbOnline = sysconf(_SC_NPROCESSORS_ONLN);
    for (long aCoreIter = 0; aCoreIter < aNbOnline; ++aCoreIter)
      CPU_SET(aCoreIter, &aCpuSet);
  }
  else if (getPerformantCoresMask(aCpuSet) == 0)
  {
    return false;
  }

  if (sched_setaffinity(getpid(), sizeof(aCpuSet), &aCpuSet) == 0)
    return true;
#else
  (void)theToSet;
#endif
  return false;
}
