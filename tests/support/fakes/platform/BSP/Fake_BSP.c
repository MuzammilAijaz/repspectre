#include "Fake_BSP.h"

static int initShouldPass;

void Fake_BSP_ctor(void) {
	initShouldPass = 1;
}

void Fake_BSP_dtor(void) {

}

void Fake_BSP_InjectError(void) {
	initShouldPass = 0;
}

int Fake_BSP_init(void) {
	// publish error
	return initShouldPass;
}


BspInterface FakeBSPinterface= {
	.BSP_init = Fake_BSP_init,
};
