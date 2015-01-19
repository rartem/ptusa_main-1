/// @file PAC_err.h
/// @brief �������� �������� �������, ������� ������������ ��� �����������
/// ������ � ������������ �������� PAC.
///
/// ����� @ref PAC_critical_errors_manager, �������� ��� ����������� ������ �
/// �������� ������� ���������, �������� ��� ����������� ����������. ��� �����
/// ���� ������ ������ ��������� @ref PAC_critical_errors_manager::critical_error.
///
/// @author  ������ ������� ���������.
///
/// @par �������� �������� �������������:
/// @c PAC_ERRORS_H - ��������� ������� ����� � ���������� ������ ���� ���.@n
///
/// @par ������� ������:
/// @$Rev$.\n
/// @$Author$.\n
/// @$Date::                     $.

#ifndef PAC_ERRORS_H
#define PAC_ERRORS_H

#include <string.h>
#include <vector>

#include "errors.h"
#include "smart_ptr.h"
#include "dtime.h"
#include "led.h"

//-----------------------------------------------------------------------------
//0, 1, n - ��� ����� � ����� Wago ����� n.
//0, 2, n - ��� ����� � ������� ����� n.
//0, 3, n - ��� ����� � Modbus ����������� ����� n.
//0, 4, 4 -
//0, 5, 5 - ��� ����� � ��������.
//
//1, 1 - ������ ������ � COM-������ WAGO.
//      1 - CRC error
//
//13 - ������ ������� ������:
//  1, n  - ������ ��������� ������ ����� n.
//-----------------------------------------------------------------------------
class PAC_critical_errors_manager
    {
    public:
        enum CONSTANTS
            {
            ALARM_CLASS_PRIORITY = 100,
            };

        enum ALARM_CLASS      ///< ����� �������.
            {
            AC_UNKNOWN,
            AC_NO_CONNECTION, ///< ������ �����.

            AC_COM_DRIVER,    ///< ������ ������ � COM-������.
            AC_RUNTIME_ERROR, ///< ������ �� ����� ������.
            };

        enum ALARM_SUBCLASS         ///< �������� �������.
            {
            //AC_NO_CONNECTION,     ///< ������ �����.
            AS_WAGO = 1,            ///< ������ ������� WAGO.
            AS_PANEL,               ///< ������ ������� EasyView.
            AS_MODBUS_DEVICE,       ///< ������ ����������, ������������� �� Modbus.

            AS_EASYSERVER = 5,      ///< ������ EasyServer.
            AS_REMOTE_PAC,          ///< ������ ����� � ��������� PAC.

            //AC_RUNTIME_ERROR,     ///< ������ �� ����� ������.
            AS_EMERGENCY_BUTTON = 1,///< ������ ��������� ������.
            };

    public:
        enum GE_CONST
            {
            GE_ERROR_SIZE = 3,      ///< ������ ����� ������, ����.
            };

        PAC_critical_errors_manager();

        void show_errors();
        void set_global_error( ALARM_CLASS eclass, ALARM_SUBCLASS p1,
            unsigned long param );
        void reset_global_error( ALARM_CLASS eclass, ALARM_SUBCLASS p1,
            unsigned long param );

        int save_as_Lua_str( char *str, u_int_2 &id );

        static PAC_critical_errors_manager * get_instance();

        u_int get_id() const
            {
            return errors_id;
            }

        bool is_any_error() const
            {
            return !errors.empty();
            }

    private:
        const char* get_alarm_group()
            {
            return "������";
            }

        const char* get_alarm_descr( ALARM_CLASS err_class,
            ALARM_SUBCLASS err_sub_class, int par )
            {
            static char tmp[ 100 ] = "";

            switch( err_class )
                {
            case AC_UNKNOWN:
                return "?";

            case AC_NO_CONNECTION:
                switch( err_sub_class )
                    {
                case AS_WAGO:
                    sprintf( tmp, 
                        "��� ����� � ����� Wago '%s' ('%s', '%s')",
                        G_WAGO_MANAGER()->get_node( par )->name,
                        G_WAGO_MANAGER()->get_node( par )->ip_address,
                        G_CMMCTR->get_host_name_rus() );
                    return tmp;

                case AS_PANEL:
                    sprintf( tmp, "��� ����� � ������� EasyView �%d", par );
                    return tmp;

                case AS_MODBUS_DEVICE:
                    sprintf( tmp, "��� ����� � Modbus-����������� �%d", par );
                    return tmp;

                case AS_EASYSERVER:
                    return "EasyServer";

                case AS_REMOTE_PAC:
                    return "Remote PAC";
                    }//switch( err_sub_class )
                break;

            case AC_COM_DRIVER:
                return "?";
                break;

            case AC_RUNTIME_ERROR:
                switch( err_sub_class )
                    {
                case AS_EMERGENCY_BUTTON:
                    sprintf( tmp, "������ ��������� ������ �%d", par );
                    return tmp;

                default:
                    return "?";
                    }// switch( err_sub_class )
                }// switch( err_class )

            return "?";
            }

        static auto_smart_ptr < PAC_critical_errors_manager > instance;

        struct critical_error
            {
            int             err_class;     ///< ����� ������.
            unsigned int    err_sub_class; ///< �������� ������.
            unsigned int    param;         ///< �������� ������.

            critical_error( int err_class = 0, u_int err_sub_class = 0,
                u_int param = 0 );
            };

        std::vector< critical_error >  errors;

        u_int_2 errors_id;       
    };
//-----------------------------------------------------------------------------
#endif // PAC_ERRORS_H
