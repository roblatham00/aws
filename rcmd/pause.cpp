#include <iostream>
#include "aws_cmd.h"

#define AWS_CMD "pause"
#define AWS_CMD_USAGE "pause"

int main(int argc, char ** argv)
{
  return aws_cmd(argc, argv, AWS_CMD);
}
