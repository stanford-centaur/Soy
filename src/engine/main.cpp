#include "Error.h"
#include "Soy.h"
#include "Options.h"

static std::string getCompiler() {
  std::stringstream ss;
#ifdef __GNUC__
  ss << "GCC";
#else  /* __GNUC__ */
  ss << "unknown compiler";
#endif /* __GNUC__ */
#ifdef __VERSION__
  ss << " version " << __VERSION__;
#else  /* __VERSION__ */
  ss << ", unknown version";
#endif /* __VERSION__ */
  return ss.str();
}

static std::string getCompiledDateTime() { return __DATE__ " " __TIME__; }

void printVersion() {
  std::cout << "Soy version " << SOY_VERSION << " [" << GIT_BRANCH
            << " " << GIT_COMMIT_HASH << "]"
            << "\ncompiled with " << getCompiler() << "\non "
            << getCompiledDateTime() << std::endl;
}

void printHelpMessage() {
  printVersion();
  Options::get()->printHelpMessage();
}

int main(int argc, char **argv) {
  try {
    Options *options = Options::get();
    options->parseOptions(argc, argv);

    if (options->getBool(Options::HELP)) {
      printHelpMessage();
      return 0;
    };

    if (options->getBool(Options::VERSION)) {
      printVersion();
      return 0;
    };

    Soy().run();
  } catch (const Error &e) {
    printf("Caught a %s error. Code: %u, Errno: %i, Message: %s.\n",
           e.getErrorClass(), e.getCode(), e.getErrno(), e.getUserMessage());

    return 1;
  }

  return 0;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
