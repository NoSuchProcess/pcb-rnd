#undef PCB_REGISTER_ACTIONS
#undef PCB_REGISTER_ATTRIBUTES

#define PCB_REGISTER_ACTIONS(a, cookie) {extern void PCB_HIDCONCAT(register_,a)();PCB_HIDCONCAT(register_,a)();}
#define PCB_REGISTER_ATTRIBUTES(a, cookie) {extern void PCB_HIDCONCAT(register_,a)();PCB_HIDCONCAT(register_,a)();}

