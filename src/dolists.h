/* $Id$ */

#undef REGISTER_ACTIONS
#undef REGISTER_ATTRIBUTES

#define REGISTER_ACTIONS(a, cookie) {extern void HIDCONCAT(register_,a)();HIDCONCAT(register_,a)();}
#define REGISTER_ATTRIBUTES(a, cookie) {extern void HIDCONCAT(register_,a)();HIDCONCAT(register_,a)();}

