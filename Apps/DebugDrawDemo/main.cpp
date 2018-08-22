﻿#include <Usagi/Engine/Runtime/Runtime.hpp>

#include "DebugDrawDemo.hpp"

using namespace usagi;

int main(int argc, char *argv[])
{
    auto runtime = Runtime::create();
    runtime->enableCrashHandler("DebugDrawDemoErrorDump");
    DebugDrawDemo demo(runtime.get());
    demo.run();
}
