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

  #ifdef __APPLE__
    #include <sys/sysctl.h>
  #endif

  #ifdef __sun
    #include <sys/processor.h>
    #include <sys/procset.h>
  #else
    #include <sched.h>
  #endif
#endif

#include <array>
#include <bitset>
#include <fstream>
#include <limits>
#include <map>
#include <vector>

#include <Standard_WarningDisableFunctionCast.hxx>

namespace
{

#if (defined(_WIN32) && !defined(OCCT_UWP)) || (defined(__linux__) && defined(__GLIBC__)) || defined(__APPLE__)
  //! Check if logical processor passes the filter or not.
  //! Note that the actual CPU architecture is not checked here, just assumed 3 performance classes of cores.
  //! @param[in] theFilter input filter
  //! @param[in] theNbClasses number of classes in CPU
  //! @param[in] theMinClass minimal class in CPU
  //! @param[in] theMaxClass maximal class in CPU
  //! @param[in] theClass class of this logical processor
  //! @return TRUE if logical processor passes the filter.
  static bool isCoreInClass(const OSD_Parallel::LogicalProcessorFilter theFilter,
                            const size_t theNbClasses,
                            const int theMinClass,
                            const int theMaxClass,
                            const int theClass)
  {
    switch (theFilter)
    {
      case OSD_Parallel::LogicalProcessorFilter::All:
        return true;
      case OSD_Parallel::LogicalProcessorFilter::Pinned:
        return false;
      case OSD_Parallel::LogicalProcessorFilter::Topmost:
        return (theClass == theMaxClass) && (theNbClasses >= 2);
      case OSD_Parallel::LogicalProcessorFilter::Lowest:
        return (theClass == theMinClass) && (theNbClasses >= 3);
      case OSD_Parallel::LogicalProcessorFilter::Performant:
      {
        if (theNbClasses >= 3)
          return (theClass != theMinClass);
        else
          return true;
      }
      case OSD_Parallel::LogicalProcessorFilter::Common:
      {
        if (theNbClasses >= 3)
          return (theClass != theMinClass) && (theClass != theMaxClass);
        else if (theNbClasses <= 2)
          return theClass == theMinClass;
      }
    }
    return false;
  }
#endif

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

  //! Computes a group 0 processor affinity mask keeping a single logical CPU
  //! per distinct core (hyper-thread siblings).
  //!
  //! The filtering is based on EfficiencyClass or SchedulingClass information.
  //!
  //! Returns the number of filtered cores.
  static int getCpuCoresMask(DWORD_PTR& theMask,
                             const OSD_Parallel::LogicalProcessorFilter theFilter,
                             const bool theIsOnePerCore)
  {
    if (theFilter == OSD_Parallel::LogicalProcessorFilter::Pinned)
      return 0; // not applicable

    typedef BOOL (WINAPI *GetSystemCpuSetInformation_t)(void*, ULONG, ULONG*, HANDLE, ULONG);
    const HMODULE aKern32 = GetModuleHandleW(L"kernel32");
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

    constexpr int aMaskBits = std::numeric_limits<DWORD_PTR>::digits;

    // first pass: check whether EfficiencyClass / SchedulingClass vary across CPU sets
    std::map<int, int> aMapOfEff;
    std::map<int, int> aMapOfSched;
    for (ULONG anOffset = 0; anOffset + sizeof(OcctCpuSetEntry) <= aBufLen;)
    {
      const OcctCpuSetEntry* anEntry = (const OcctCpuSetEntry* )(aBuffer.data() + anOffset);
      if (anEntry->Size < sizeof(OcctCpuSetEntry) || anOffset + anEntry->Size > aBufLen)
        break;

      const int anEff  = (int )anEntry->EfficiencyClass;
      const int aSched = (int )anEntry->SchedulingClass;
      ++aMapOfEff[anEff];
      ++aMapOfSched[aSched];

      anOffset += anEntry->Size;
    }

    const size_t aNbEff   = aMapOfEff.size();
    const size_t aNbSched = aMapOfSched.size();

    const int aMinEff = !aMapOfEff.empty() ? aMapOfEff.begin()->first : 0;
    const int aMaxEff = !aMapOfEff.empty() ? aMapOfEff.rbegin()->first : 0;
    const int aMinSched = !aMapOfSched.empty() ? aMapOfSched.begin()->first : 0;
    const int aMaxSched = !aMapOfSched.empty() ? aMapOfSched.rbegin()->first : 0;

    // second pass: keep the lowest logical CPU index of every performance core
    std::array<int, 256> aMinLpiPerCore;
    for (int& aCoreMinLpi : aMinLpiPerCore)
      aCoreMinLpi = -1;

    theMask = 0;
    int aNbSel = 0;
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

      // Use SchedulingClass only when it has expected range of known Intel hybrid CPUs:
      // - 2 EfficiencyClass (E+P-cores) with 3 SchedulingClass (E-cores + common P-cores + boosted P-cores).
      // Other non-hybrid CPUs (AMD Ryzen) report unique SchedulingClass for every distinct core with no correlation.
      // Hence, EfficiencyClass is more predictable (tested CPUs report only 1 or 2 classes).
      const bool toInclude = (aNbSched == 3 && aNbEff == 2)
                           ? isCoreInClass(theFilter, aNbSched, aMinSched, aMaxSched, aSched)
                           : isCoreInClass(theFilter, aNbEff, aMinEff, aMaxEff, anEff);
      if (!toInclude)
        continue;

      if (theIsOnePerCore)
      {
        if (aMinLpiPerCore[aCore] < 0 || aLpi < aMinLpiPerCore[aCore])
          aMinLpiPerCore[aCore] = aLpi;

        continue;
      }

      theMask |= ((DWORD_PTR )1 << aLpi);
      ++aNbSel;
    }
    if (!theIsOnePerCore)
      return aNbSel;

    for (size_t aCoreIter = 0; aCoreIter < aMinLpiPerCore.size(); ++aCoreIter)
    {
      if (aMinLpiPerCore[aCoreIter] >= 0)
      {
        theMask |= ((DWORD_PTR )1 << aMinLpiPerCore[aCoreIter]);
        ++aNbSel;
      }
    }
    return aNbSel;
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
  //! Fills @p theCpuSet with one logical CPU filtered by specified criteria.
  //!
  //! Online CPUs are grouped by physical core (physical_package_id:core_id).
  //! The filtering is based on frequency information (cpufreq/cpuinfo_max_freq) available in procfs.
  //!
  //! Returns the number of filtered cores.
  static int getCpuCoresMask(cpu_set_t& theCpuSet,
                             const OSD_Parallel::LogicalProcessorFilter theFilter,
                             const bool theIsOnePerCore)
  {
    if (theFilter == OSD_Parallel::LogicalProcessorFilter::Pinned)
      return 0; // not applicable

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
      std::vector<int> CpuIds;
      int MaxFreq = 0;
    };
    std::map<PackageAndCore, CoreInfo> aMapOfCores;
    std::map<int, int> aMapOfFreq;

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

      const PackageAndCore aKey(aPkgId, aCoreId);
      auto aCoreIter = aMapOfCores.find(aKey);
      if (aCoreIter == aMapOfCores.end())
      {
        CoreInfo& anInfo = aMapOfCores[aKey];
        anInfo.CpuIds.push_back(aCpuId);
        anInfo.MaxFreq = aCoreFreq;

        // count hyper-threaded core only once
        ++aMapOfFreq[aCoreFreq];
      }
      else
      {
        CoreInfo& anInfo = aCoreIter->second;
        anInfo.CpuIds.push_back(aCpuId);
        if (aCoreFreq > anInfo.MaxFreq)
          anInfo.MaxFreq = aCoreFreq;

        // put the lowest logical CPU within hyper-threaded core first
        if (aCpuId < anInfo.CpuIds.front())
          std::swap(anInfo.CpuIds.front(), anInfo.CpuIds.back());
      }
    }

    if (aMapOfCores.empty())
      return 0;

    // Usual grades in heterogeneous CPU configuration:
    // - Many energy-efficient E-cores (lowest frequency, no hyper-threading);
    // - Many normal performance P-cores (average frequency, hyper-threading);
    // - A couple of accelerated P-cores (highest frequency, optimized for single-threaded tasks - small amount).
    // Example: 12 E-cores + 8 P-cores (including 2 accelerated cores; hyper-threaded), 12+8x2=28 logical CPUs.
    const size_t aNbClasses = aMapOfFreq.size();
    const int aMinEff = !aMapOfFreq.empty() ? aMapOfFreq.begin()->first : 0;
    const int aMaxEff = !aMapOfFreq.empty() ? aMapOfFreq.rbegin()->first : 0;

    int aNbSel = 0;
    for (const std::pair<const PackageAndCore, CoreInfo>& aCoreIter : aMapOfCores)
    {
      const CoreInfo& anInfo = aCoreIter.second;
      const bool toInclude = isCoreInClass(theFilter, aNbClasses, aMinEff, aMaxEff, anInfo.MaxFreq);
      if (!toInclude)
        continue;

      for (int aCpuIdIter : anInfo.CpuIds)
      {
        CPU_SET(aCpuIdIter, &theCpuSet);
        ++aNbSel;
        if (theIsOnePerCore)
          break;
      }
    }
    return aNbSel;
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
    const int aNbOnline = NbLogicalProcessors(LogicalProcessorFilter::All, false);
    const int aNbPinned = NbLogicalProcessors(LogicalProcessorFilter::Pinned, false);
    return aNbPinned != 0 ? aNbPinned : aNbOnline;
  };

  const int aNbProcessors = getNbPinned();
  return aNbProcessors;
}

//=======================================================================
//function : NbLogicalProcessors
//purpose  :
//=======================================================================
int OSD_Parallel::NbLogicalProcessors(const LogicalProcessorFilter theFilter,
                                      const bool theIsOnePerCore)
{
#ifdef _WIN32
  // Fetch CPU affinity mask for systems having less than 64 cores:
  // - Win7+ support more than 64 cores, split into 'groups',
  //   process is assigned to one group by default (more groups could be exposed via dedicated API);
  // - Win11+ changed behavior so that process may utilize more than 64 cores by default,
  //   with old 'groups' API mixed up with new 'SelectedCpuSetMasks' API.
  // Affinity mask is ignored if it is full or empty.
  if (theFilter == LogicalProcessorFilter::Pinned)
  {
    DWORD_PTR aProcAff = 0, aSysAff = 0;
    const HANDLE aProcess = GetCurrentProcess();
    if (GetProcessAffinityMask(aProcess, &aProcAff, &aSysAff))
    {
      constexpr int aMaxAff = std::numeric_limits<DWORD_PTR>::digits;
      const     int aNbAff  = (int )std::bitset<aMaxAff>(aProcAff).count();
      if (aNbAff > 0 && aNbAff != aMaxAff)
        return aNbAff;
    }
  }
  else if (theFilter != LogicalProcessorFilter::All || theIsOnePerCore)
  {
    DWORD_PTR aMask = 0;
    return getCpuCoresMask(aMask, theFilter, theIsOnePerCore);
  }

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
  return aSysInfo.dwNumberOfProcessors;
#else

#if defined(__ANDROID__)
  if (theFilter == LogicalProcessorFilter::All
   || theFilter == LogicalProcessorFilter::Pinned)
  {
    uint32_t aCpuMaskPresent  = readCpuMask("/sys/devices/system/cpu/present");
    uint32_t aCpuMaskPossible = readCpuMask("/sys/devices/system/cpu/possible");
    aCpuMaskPresent &= aCpuMaskPossible;
    const int aNbPresent = __builtin_popcount(aCpuMaskPresent);
    if (aNbPresent >= 1)
      return aNbPresent;
  }
#endif

  // These are the choices. We'll check number of processors online.
  // _SC_NPROCESSORS_CONF   Number of processors configured
  // _SC_NPROCESSORS_MAX    Max number of processors supported by platform
  // _SC_NPROCESSORS_ONLN   Number of processors online
  const long aNbOnline = sysconf(_SC_NPROCESSORS_ONLN);

#if defined(__APPLE__)
  if (theFilter == LogicalProcessorFilter::Pinned)
  {
    // not supported by platform
  }
  else if (theFilter != LogicalProcessorFilter::All || theIsOnePerCore)
  {
    const auto getSysctlValue = [](int& theVal, const TCollection_AsciiString& theName) -> bool
    {
      size_t aSize = sizeof(theVal);
      return sysctlbyname(theName.ToCString(), &theVal, &aSize, nullptr, 0) == 0;
    };

    int aNbPerfLevels = 0;
    if (getSysctlValue(aNbPerfLevels, "hw.nperflevels") && aNbPerfLevels > 0)
    {
      // performance levels are sorted from fastest to slowest
      int aNbSel = 0;
      for (int aLevelIter = 0; aLevelIter < aNbPerfLevels; ++aLevelIter)
      {
        const TCollection_AsciiString aPrefix =
          TCollection_AsciiString("hw.perflevel") + aLevelIter + ".";

        // Each level has a marketing name like "Efficiency"+"Performance" for M1 to M4,
        // or "Performance"+"Super" since M5,
        // so just use general ranging based on the number of levels.
        //getSysctlString(aName, aPrefix + "name");

        const bool toInclude = isCoreInClass(theFilter, aNbPerfLevels, 1, aNbPerfLevels, aNbPerfLevels - aLevelIter);
        if (!toInclude)
          continue;

        int aNbCores = 0, aNbLogical = 0;
        getSysctlValue(aNbCores, aPrefix + "physicalcpu");
        getSysctlValue(aNbLogical, aPrefix + "logicalcpu");
        aNbSel += theIsOnePerCore ? aNbCores : aNbLogical;
      }
      return aNbSel;
    }
    else
    {
      // older (Intel) CPUs without performance levels
      if (!isCoreInClass(theFilter, 1, 1, 1, 1))
        return 0;

      int aNbCores = 0, aNbLogical = 0;
      getSysctlValue(aNbCores, "hw.physicalcpu");
      getSysctlValue(aNbLogical, "hw.logicalcpu");
      return theIsOnePerCore ? aNbCores : aNbLogical;
    }
  }
#elif defined(__linux__) && defined(__GLIBC__)
  cpu_set_t aCpuSet;
  CPU_ZERO(&aCpuSet);
  if (theFilter == LogicalProcessorFilter::Pinned)
  {
    if (sched_getaffinity(getpid(), sizeof(aCpuSet), &aCpuSet) == 0)
    {
      int aNbAff = 0;
      for (long aCoreIter = 0; aCoreIter < aNbOnline; ++aCoreIter)
      {
        if (CPU_ISSET(aCoreIter, &aCpuSet))
          ++aNbAff;
      }
      if (aNbAff != 0)
        return aNbAff;
    }
  }
  else if (theFilter != LogicalProcessorFilter::All || theIsOnePerCore)
  {
    CPU_ZERO(&aCpuSet);
    return getCpuCoresMask(aCpuSet, theFilter, theIsOnePerCore);
  }
#else
  (void)theIsOnePerCore;
  if (theFilter != LogicalProcessorFilter::All
   && theFilter != LogicalProcessorFilter::Pinned)
    return 0;
#endif

  return int(aNbOnline);
#endif
}

//=======================================================================
//function : SetLogicalProcessorsAffinity
//purpose  :
//=======================================================================
bool OSD_Parallel::SetLogicalProcessorsAffinity(const LogicalProcessorFilter theFilter,
                                                const bool theIsOnePerCore)
{
  if (theFilter == LogicalProcessorFilter::Pinned)
    return false;

#if defined(_WIN32) && !defined(OCCT_UWP)
  DWORD_PTR aMask = 0;
  if (theFilter == LogicalProcessorFilter::All)
  {
    DWORD_PTR aProcAff = 0;
    const HANDLE aProcess = GetCurrentProcess();
    GetProcessAffinityMask(aProcess, &aProcAff, &aMask);
  }
  else if (getCpuCoresMask(aMask, theFilter, theIsOnePerCore) == 0)
  {
    return false;
  }

  if (SetProcessAffinityMask(GetCurrentProcess(), aMask))
    return true;
#elif defined(__linux__) && defined(__GLIBC__)
  cpu_set_t aCpuSet;
  CPU_ZERO(&aCpuSet);
  if (theFilter == LogicalProcessorFilter::All)
  {
    long aNbOnline = sysconf(_SC_NPROCESSORS_ONLN);
    for (long aCoreIter = 0; aCoreIter < aNbOnline; ++aCoreIter)
      CPU_SET(aCoreIter, &aCpuSet);
  }
  else if (getCpuCoresMask(aCpuSet, theFilter, theIsOnePerCore) == 0)
  {
    return false;
  }

  if (sched_setaffinity(getpid(), sizeof(aCpuSet), &aCpuSet) == 0)
    return true;
#else
  (void)theIsOnePerCore;
#endif
  return false;
}
