//
// Testing Com_Parse()
//
#include "qcommon/q_shared.h"

#include <cstring>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

void Com_Error( int level, const char *error, ... ) { exit(0); }
void Com_Printf( const char *msg, ... ) {}

TEST_CASE("validate Com_sprintf", "[Com_sprintf]")
{
    char command[1024];
    
    SECTION("populating an uninitialized array")
    {
        Com_sprintf(command, sizeof(command), "sv_master%d", 1);
        REQUIRE(!strcmp(command, "sv_master1"));
    }

    SECTION("overwritting array contents")
    {
        Com_sprintf(command, sizeof(command), "sv_master%d", 2);
        REQUIRE(!strcmp(command, "sv_master2"));
    }
}


