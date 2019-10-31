#undef PCB_REGISTER_ACTIONS

#define PCB_REGISTER_ACTIONS(a, cookie) {extern void PCB_HIDCONCAT(register_,a)();PCB_HIDCONCAT(register_,a)();}


