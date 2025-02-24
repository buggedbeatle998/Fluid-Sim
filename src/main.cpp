#include <vk_engine.h>
#include <simu.h>

int main(int argc, char* argv[])
{
	Fluid fluid;
	VulkanEngine engine;

	engine.init(&fluid);
	fluid.init(&engine);
	
	engine.run();	

	fluid.cleanup();
	engine.cleanup();	

	return 0;
}
