#include <vk_engine.h>
#include <simu.h>

int main(int argc, char* argv[])
{
	Fluid fluid;
	VulkanEngine engine;

	fluid.init();
	engine.init(&fluid);
	
	engine.run();	

	engine.cleanup();	
	fluid.cleanup();

	return 0;
}
