/*
    Core 1 is dedicated amongst many different tasks, including (but not
    limited to):
        USB device management
            Input event management
            Sound transmission*
            Networking*
        Netplay (IC2)*
    USB device management will ALWAYS be delegated to Core 1 and will always be running
    However, provided that that Sound and USB Network are not active, then all USB activity
    may be implemented and handled using IRQs. As such, only when USB is the only Core 1
    task may rendering also take place, if at any point Sound or any form of Networking
    enabled, then rendering on Core 1 will cease.

    In such a case, IRQs are no longer sufficient to handle all USB activity, and Core 1
    will transition to a "Round Robin" Task Scheduler between all active tasks. Tasks will
    not create a "task struct" as in normal operating system, and as such, new tasks other
    than the ones specified above (or otherwise defined in this project) have no way to be
    specified

    "Round Robin" scheduler:
        This isnt a proper scheduler, but instead merely a series of conditional ifs that
        will pass and continue to "scheduled" (active) tasks
*/

int main() {

}