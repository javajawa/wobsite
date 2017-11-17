#include "layouts/example.h"
#include "layouts/template.h"

const union html_node example = element( "p",
	attrs( at_str( "id", "foo" ), at_rep( "class", CLASSNAME ) ),
	children(
		text( "Hello, " ),
		element( "b", NULL, children( replace( TO_GREET ) ) ),
		element( "i", NULL, NULL )
	)
);
