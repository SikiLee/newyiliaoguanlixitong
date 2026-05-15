#include <stdio.h>
#include "gui.h"
#include "test.h"
#include "file.h"

int main(void)
{
#ifdef USE_GUI
    runGui();
#else
    char out[30000];
    char msg[256];
    loadAllData("data", msg, sizeof(msg));
    printf("%s\n\n", msg);
    runAllTests(out, sizeof(out));
    printf("%s\n", out);
#endif
    return 0;
}
