#pragma once
#define FACTORY(NAMESPACE, CLASS) \
namespace NAMESPACE::factory_implementation \
{ \
	struct CLASS : CLASS ## T<CLASS, implementation::CLASS> \
	{ \
	}; \
}
