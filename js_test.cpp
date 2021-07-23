/*
simple joystick test program.
author: tanganke, 2021-7-23
reference: https://www.kernel.org/doc/Documentation/input/joystick-api.txt
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>

struct js_status
{
  int16_t *axis;
  int16_t *button;

  js_status() = default;
  js_status(uint8_t num_axis, uint8_t num_button)
  {
    axis = new int16_t[num_axis];
    button = new int16_t[num_button];
  }
  ~js_status()
  {
    delete[] axis;
    delete[] button;
  }
};

void js_status_init(js_status *status, uint8_t num_axis, uint8_t num_button)
{
  status->axis = new int16_t[num_axis];
  status->button = new int16_t[num_button];
}

void js_status_free(js_status *status)
{
  delete[] status->axis;
  delete[] status->button;
}

void js_status_print(js_status *status, uint8_t num_axis, uint8_t num_button)
{
  for (int i = 0; i < num_axis; ++i)
  {
    std::cout << "axis " << i << ':' << status->axis[i] << ' ';
  }
  for (int i = 0; i < num_button; ++i)
  {
    std::cout << "button " << i << ':' << status->button[i] << ' ';
  }
  std::cout << '\n';
}

int main(int argc, const char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "joystick test program:\n"
              << "usage example:\n"
              << "\tjs_test /dev/input/js0" << std::endl;
    exit(EXIT_FAILURE);
  }

  int js_fd = open(argv[1], O_RDONLY);
  if (js_fd == -1)
  {
    std::cerr << "cannot open file: " << argv[1] << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string js_name;
  char js_num_axis;
  char js_num_buttons;

  {
    char *name = (char *)malloc(128);
    if (ioctl(js_fd, JSIOCGNAME(128), name) < 0)
      strcpy(name, "Unknown");
    js_name = name;
    free(name);
  }
  ioctl(js_fd, JSIOCGAXES, &js_num_axis);
  ioctl(js_fd, JSIOCGBUTTONS, &js_num_buttons);
  std::cout << "joystick information:\n"
            << "- js_name: " << js_name << '\n'
            << "- js_num_axis: " << (int)js_num_axis << '\n'
            << "- js_num_buttons: " << (int)js_num_buttons << '\n'
            << std::flush;

  js_status js_s(js_num_axis, js_num_buttons);
  js_event js_e;
  while (true)
  {
    ssize_t s = read(js_fd, &js_e, sizeof(js_event));

    // error handling.
    if (s != sizeof(js_event))
    {
      std::cerr << "fail to reading js_event, reconnecting...\n";
      if (js_fd != -1)
      {
        close(js_fd);
        js_fd = -1;
      }

      int retry_count = 0;
      while (js_fd == -1)
      {
        retry_count++;
        js_fd = open(argv[1], O_RDONLY);
        if (js_fd == -1)
        {
          std::cerr << '(' << retry_count << ')'
                    << " cannot open file: " << argv[1] << '\n';
          sleep(1);
        }
      }
      continue;
    }

    // handle js_event hereafter
    if (js_e.type & JS_EVENT_BUTTON)
    {
      js_s.button[js_e.number] = js_e.value;
    }
    else if (js_e.type & JS_EVENT_AXIS)
    {
      js_s.axis[js_e.number] = js_e.value;
    }

    js_status_print(&js_s, js_num_axis, js_num_buttons);
  }
}
