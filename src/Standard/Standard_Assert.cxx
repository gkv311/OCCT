// Created on: 2025-11-23
// Created by: Andrey Betenev
// Public domain

#include <Standard_Assert.hxx>

// define a macro Standard_ASSERT_DBGBREAK_() causing debugger break based on current system
#if defined(_WIN32)

#include <windows.h>
#include <debugapi.h>

//#if defined(_MSC_VER) && defined(_DEBUG)
//  #include <crtdbg.h> // _CrtDbgReport
//#endif

#if defined(_MSC_VER) || defined(__MINGW64__)
  // VS-specific intrinsic
  #define Standard_ASSERT_DBGBREAK_() __debugbreak()
#else
  // WinAPI function
  #define Standard_ASSERT_DBGBREAK_() DebugBreak()
#endif

#elif defined(__EMSCRIPTEN__)
  #include <emscripten.h>
  #define Standard_ASSERT_DBGBREAK_() emscripten_debugger()
#else
  // POSIX systems
  #include <signal.h>
  #define Standard_ASSERT_DBGBREAK_() raise(SIGTRAP)
#endif

#if defined(__APPLE__)
  #include <sys/sysctl.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

namespace
{

  enum OcctAssertMode
  {
    OcctAssert_Abort,  //!< "abort"  print message to cerr and call abort(1)
    OcctAssert_Break,  //!< "break"  print message to cerr and raise debugger break
    OcctAssert_Debug,  //!< "debug"  (default) print message to cerr and raise debugger break,
    OcctAssert_Report, //!< "report" only print report message to cerr and continue
    OcctAssert_Quiet   //!< "quiet"  do nothing, do not even print the report message
  };

  static OcctAssertMode GetAssertMode()
  {
    // do not use OSD_Environment to avoid cyclic dependency from OSD
    const char* aVar = ::getenv("CSF_ASSERT_MODE");
    if (aVar == nullptr)
      return OcctAssert_Debug;

    if (strcasecmp(aVar, "abort") == 0)
      return OcctAssert_Abort;
    else if (strcasecmp(aVar, "break") == 0)
      return OcctAssert_Break;
    else if (strcasecmp(aVar, "debug") == 0)
      return OcctAssert_Debug;
    else if (strcasecmp(aVar, "report") == 0)
      return OcctAssert_Report;
    else if (strcasecmp(aVar, "quiet") == 0)
      return OcctAssert_Quiet;

    std::cerr << "Error: environment variable CSF_ASSERT_MODE has unsupported value \"" << aVar << "\"; "
              << "should be one of \"abort\", \"break\", \"debug\", \"report\", or \"quiet\". "
              << "See Standard_Assert.hxx for details.\n";
    return OcctAssert_Debug;
  }

#ifdef _WIN32
  // IsDebuggerPresent() is defined by "debugapi.h"

#elif defined(__EMSCRIPTEN__)
  // Assume that debugger is always present in WinASM
  static bool IsDebuggerPresent()
  {
    return true;
  }

#elif (defined(__linux__) || defined(__linux))
  // Detect presence of debugger on Linux by looking for TracerPid in "/proc/self/status"
  static bool IsDebuggerPresent()
  {
    static constexpr char THE_PROC_SELF_STATUS[] = "/proc/self/status";
    static constexpr char THE_TracerPid[] = "TracerPid:";
    static constexpr int  THE_TracerPid_LEN = 10;

    // use the same logic for parsing procfs as in OSD_MemInfo::Update()
    std::ifstream aFile;
    aFile.open(THE_PROC_SELF_STATUS);
    if (!aFile.is_open())
      return false;

    char aBuff[512];
    while (!aFile.eof())
    {
      memset(aBuff, 0, sizeof(aBuff));
      aFile.getline(aBuff, sizeof(aBuff));
      if (aBuff[0] == '\0')
        continue;

      if (strncmp(aBuff, THE_TracerPid, THE_TracerPid_LEN) != 0)
        continue;

      const long aTracerPid = atol(aBuff + THE_TracerPid_LEN);
      return aTracerPid != 0;
    }
    return false;
  }
#elif defined(__APPLE__)
  // Detect presence of debugger on macOS by looking for P_TRACED flag
  static bool IsDebuggerPresent()
  {
    int aMib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };

    struct kinfo_proc aKInfo = {};
    size_t aKInfoSize = sizeof(aKInfo);

    const int isOk = sysctl(aMib, sizeof(aMib) / sizeof(*aMib),
                            &aKInfo, &aKInfoSize, nullptr, 0);
    if (isOk != 0)
      return false; // something is wrong

    return (aKInfo.kp_proc.p_flag & P_TRACED) != 0;
  }
#else
  // Assume no debugger
  static bool IsDebuggerPresent()
  {
    return false;
  }
#endif
}

void Standard_ASSERT_HANDLER(const char* theFile, int theLine, const char* theExpr, const char* theDesc)
{
  static const OcctAssertMode aMode = GetAssertMode();
  if (aMode == OcctAssert_Quiet)
    return;

  // print error message
  std::cerr << "ERROR: assertion failed, statement '" << theExpr << "' is not TRUE!\n"
            << " File: '" << theFile << "'\n"
            << " Line: " << theLine << "\n";
  if (theDesc != nullptr && *theDesc != '\0')
    std::cerr << " Description: " << theDesc << "\n";

  std::cerr << std::flush;
  if (aMode == OcctAssert_Report)
    return;

  if (aMode == OcctAssert_Abort)
  {
    std::cerr << "Exiting the program" << std::flush;
    ::exit(1);
  }

  // in default "debug" mode, check whether we are under debugger, and if not, return
  if (aMode == OcctAssert_Debug && !IsDebuggerPresent())
    return;

#if defined(_MSC_VER) && defined(_DEBUG)
  // when running on Windows with Debug CRT but without debugger, ask the user interactively
  // whether to start debugger (opens dialog with Abort / Retry / Ignore buttons)
  // 1 means the user pressed Retry button
  //if (_CrtDbgReport(_CRT_ASSERT, theFile, theLine, NULL, "%s\n(Condition: \"%s\")\n", theDesc, theExpr) != 1) { return; }
#endif

  Standard_ASSERT_DBGBREAK_();
}
