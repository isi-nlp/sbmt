# include "distortion_model.hpp"

////////////////////////////////////////////////////////////////////////////////

using namespace sbmt;

struct distortion_init {
    distortion_init()
    {
        register_info_factory_constructor("distortion", distortion_constructor<>());
        register_rule_property_constructor("distortion","cross",read_cross());
	}
};

static distortion_init d;

////////////////////////////////////////////////////////////////////////////////

