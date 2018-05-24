// main_control_dll_vc.cpp: определяет экспортированные функции для приложения DLL.
//
#include <stdlib.h>
#include "fcntl.h"

#include "dtime.h"

#include "prj_mngr.h"
#include "PAC_dev.h"
#include "PAC_info.h"
#include "tech_def.h"
#include "lua_manager.h"
#include "PAC_err.h"

#include "rm_manager.h"
#include "log.h"
#ifdef PAC_WAGO_750_860
#include "l_log.h"
#endif

#include "profibus_slave.h"

int G_DEBUG = 0; //Вывод дополнительной отладочной информации.

int lua_init( lua_State* L )
    {
    int top = lua_gettop( L );
    const int argc = top;
    const char **argv = ( top == 0 ? new const char*[ 1 ] : new const char*[ argc ] );
    const char *empty_par = "";

    argv[ 0 ] = empty_par;

    for ( int i = 1; i <= top; i++ ) 
        {  /* repeat for each level */
        int t = lua_type( L, i );
        switch ( t ) 
            {
            case LUA_TSTRING:  /* strings */                
                argv[ i - 1 ] = lua_tostring( L, i );
                break;

            default:  /* other values */  
                argv[ i - 1 ] = empty_par;
                break;
            }
        }      

    sprintf( G_LOG->msg, "Program started." );
    G_LOG->write_log( i_log::P_INFO );

    G_PROJECT_MANAGER->proc_main_params( argc, argv );

    int res = G_LUA_MANAGER->init( L, argv[ 0 ] ); //-Инициализация Lua.

    if ( res ) //-Ошибка инициализации.
        {
        sprintf( G_LOG->msg, "Lua init returned error code %d!", 1 );
        G_LOG->write_log( i_log::P_CRIT );

        debug_break;
        return EXIT_FAILURE;
        }

#ifdef USE_PROFIBUS
    if ( G_PROFIBUS_SLAVE()->is_active() )
        {
        G_PROFIBUS_SLAVE()->init();
        }
#endif // USE_PROFIBUS

    long int sleep_time_ms = 2;
    //if ( argc >= 3 )
    //    {
    //    char *stopstring;
    //    sleep_time_ms = strtol( argv[ 2 ], &stopstring, 10 );
    //    }

    sprintf( G_LOG->msg, "Starting main loop! Sleep time is %li ms.",
        sleep_time_ms );
    G_LOG->write_log( i_log::P_INFO );


    return 0;
    }

//Регистрация реализованных в dll функций, что бы те стали доступны из lua.
struct luaL_reg ls_lib[] = 
    {
    { "init", lua_init },
    { NULL, NULL },
    };

extern "C" int __declspec( dllexport ) luaopen_main_control( lua_State* L )
    {
    luaL_openlib( L, "main_control", ls_lib, 0 );
    return 0;
    }
