#include "xparse/application.h"

int main(int argc, char* argv[])
{
    xparse::Application app;
    return app.Run(argc, argv);
}