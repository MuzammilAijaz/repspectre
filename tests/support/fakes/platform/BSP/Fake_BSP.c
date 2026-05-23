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

void Fake_BSP_configureI2cBus(void) {

}

BspInterface FakeBSPinterface= {
	.BSP_init = Fake_BSP_init,
	.BSP_configureI2cBus = Fake_BSP_configureI2cBus,
};
