#define NOB_IMPLEMENTATION

#include "thirdparty/nob.h"

#define SRC_FOLDER "src/"

int main(int argc, char *argv[]) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  Nob_Cmd cmd = {0};

  nob_cmd_append(&cmd, "cc");
  nob_cmd_append(&cmd, "-Wall");
  nob_cmd_append(&cmd, "-Wextra");
  nob_cmd_append(&cmd, "-o");
  nob_cmd_append(&cmd, "main");
  nob_cmd_append(&cmd, SRC_FOLDER "main.c");

  if (!nob_cmd_run(&cmd))
    return 1;

  nob_cmd_append(&cmd, "./main");
  if (!nob_cmd_run(&cmd))
    return 1;

  return 0;
}
