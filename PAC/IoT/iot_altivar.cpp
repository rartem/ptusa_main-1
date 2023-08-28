#include "iot_altivar.h"
#include "modbus_client.h"


altivar_manager::~altivar_manager()
	{
	if (nodes.size() > 0)
		{
		altivar_node_map::iterator it;
		for (it = nodes.begin(); it != nodes.end(); it++)
			{
			delete it->second;
			}
		}

	}

altivar_manager* altivar_manager::get_instance()
	{
	if (instance.is_null())
		{
		instance = new altivar_manager();
		}
	return instance;
	}

void altivar_manager::add_node(const char* nodeName, const char* IP_address, unsigned int port, unsigned int timeout, const char* article)
	{
	int type = altivar_node::TYPE_ATV320;
	std::string nodeArticle = std::string(article);
	if (nodeArticle.find("630") != std::string::npos)
		{
		type = altivar_node::TYPE_ATV630;
		}
	altivar_node* new_node = new altivar_node(SOCKID_ALTIVAR + index, IP_address, port, timeout, type);
	nodes.emplace(nodeName, new_node);
	index++;
	}

altivar_node * altivar_manager::get_node(const char* nodeName)
	{
	altivar_node_map::iterator it;
	it = nodes.find(nodeName);
	if (it != nodes.end())
		{
		return it->second;
		}
	else
		{
		return nullptr;
		}
	}

void altivar_manager::evaluate()
	{
	if (nodes.size() > 0)
		{
		altivar_node_map::iterator it;
		for (it = nodes.begin(); it != nodes.end(); it++)
			{
			it->second->Evaluate();
			}
		}
	}

altivar_manager::altivar_manager()
	{
	}

auto_smart_ptr< altivar_manager > altivar_manager::instance;

unsigned int altivar_manager::index = 0;

altivar_manager * G_ALTIVAR_MANAGER()
	{
	return altivar_manager::get_instance();
	}


altivar_node::altivar_node(unsigned int id, const char* ip, unsigned int port, unsigned long exchangetimeout, int type) : 
	type(type)
	{
	mc = new modbus_client(id, (char*)ip, port, exchangetimeout);
	strcpy(ip_address, ip);
	configure = true;
	querystep = RUN_STEP_CHECK_CONFIG;
	configurestep = 0;
	enabled = true;
	queryinterval = 200;
    minErrorTimeout = 60000;
    errorTimer = get_millisec();
    errorCountToReconfigure = 10;
    errorCount = 0;
	querytimer = get_millisec();
	reverse = 0;
	modbustimeout = get_millisec();
	ismodbuserror = 0;
	cmd = 4;
	state = 0;
	frq_setpoint = 0;
	frq_value = 0;
	rpm_value = 0;
	rpm_setpoint = 0;
	frq_max = FRQ_MAX_SETTING;
	amperage = 0;
	remote_state = 0;
	}

altivar_node::~altivar_node()
	{
	delete mc;
	}

void altivar_node::Evaluate()
	{
#ifndef DEBUG_NO_IO_MODULES
	int commres;
	int start_addr;
	if (enabled)
		{
		switch (querystep)
			{
			case RUN_STEP_CHECK_CONFIG:
                lblRunStepCheckConfig:
				if (configure && type == TYPE_ATV320)
					{
					configurestep = CFG_STEP_INIT_OUTPUTS;
					querystep = RUN_STEP_CONFIG;
                    goto lblRunStepConfig;
					}
				else
					{
					querystep = RUN_STEP_INIT_IOSCANNER;
                    goto lblRunStepInitIoScanner;
					}
				break;
			case RUN_STEP_CONFIG:
                lblRunStepConfig:
				switch (configurestep)
					{
					case CFG_STEP_INIT_OUTPUTS:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 8501);	//CMD(Command word) --default
						mc->set_int2(1, 8502);	//LFR (Frequency setpoint)
						mc->set_int2(2, 0);
						mc->set_int2(3, 0);
						mc->set_int2(4, 0);
						mc->set_int2(5, 0);
						configurestep = CFG_STEP_SET_OUTPUTS;
                        goto lblCfgStepSetOutputs;
						break;
					case CFG_STEP_SET_OUTPUTS:
                        lblCfgStepSetOutputs:
						if (mc->async_write_multiply_registers(15421, 6)) //Configure ouputs for modbus-scanner
							{
							configurestep = CFG_STEP_INIT_INPUTS;
                            goto lblCfgStepInitInputs;
							}
						break;

					case CFG_STEP_INIT_INPUTS:
                        lblCfgStepInitInputs:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 3201);	//ETA(Status word) --default
						mc->set_int2(1, 8604);	//RFRD (Output velocity) --default
						mc->set_int2(2, 3202);	//RFR (Output frequency)
						mc->set_int2(3, 3104);	//HSP (High speed)
						mc->set_int2(4, 3204);	//LCR (Motor current)
						mc->set_int2(5, 0);
						configurestep = CFG_STEP_SET_INPUTS;
                        goto lblCfgStepSetInputs;
					case CFG_STEP_SET_INPUTS:
                        lblCfgStepSetInputs:
						if (mc->async_write_multiply_registers(15401, 6)) //Configure inputs for modbus-scanner
							{
							configurestep = CFG_STEP_INIT_IOSCANNER;
                            goto lblCfgStepInitIoScanner;
							}
						break;
					case CFG_STEP_INIT_IOSCANNER:
                        lblCfgStepInitIoScanner:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 1);
						configurestep = CFG_STEP_SET_IOSCANNER;
                        goto lblCfgStepSetIoScanner;
					case CFG_STEP_SET_IOSCANNER:
                        lblCfgStepSetIoScanner:
						if (mc->async_write_multiply_registers(64239, 1)) //Enable IO scanner
							{
							configurestep = CFG_STEP_INIT_IOPROFILE;
                            goto lblCfgStepInitIoProfile;
							}
						break;

					case CFG_STEP_INIT_IOPROFILE:
                        lblCfgStepInitIoProfile:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 3);
						configurestep = CFG_STEP_SET_IOPROFILE;
                        goto lblCfgStepSetIoProfile;
					case CFG_STEP_SET_IOPROFILE:
                        lblCfgStepSetIoProfile:
						if (mc->async_write_multiply_registers(8401, 1)) //Setting IO-profile
							{
							configurestep = CFG_STEP_INIT_REF1;
                            goto lblCfgStepInitRef1;
							}
						break;

					case CFG_STEP_INIT_REF1:
                        lblCfgStepInitRef1:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 169);
						configurestep = CFG_STEP_SET_REF1;
                        goto lblCfgStepSetRef1;
					case CFG_STEP_SET_REF1:
                        lblCfgStepSetRef1:
						if (mc->async_write_multiply_registers(8413, 1)) //Control frequency via modbus
							{
							configurestep = CFG_STEP_INIT_CMD1;
                            goto lblCfgStepInitCmd1;
							}
						break;

					case CFG_STEP_INIT_CMD1:
                        lblCfgStepInitCmd1:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 30);
						configurestep = CFG_STEP_SET_CMD1;
                        goto lblCfgStepSetCmd1;
					case CFG_STEP_SET_CMD1:
                        lblCfgStepSetCmd1:
						if (mc->async_write_multiply_registers(8423, 1)) //Command channel via modbus
							{
							configurestep = CFG_STEP_INIT_FAULTRESET;
                            goto lblCfgStepInitFaultReset;
							}
						break;

					case CFG_STEP_INIT_FAULTRESET:
                        lblCfgStepInitFaultReset:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 1);		//Enable automatic restart
						mc->set_int2(1, 0);		//automatic restart timeout
						mc->set_int2(2, 210);	//Bind error reset to 2 bit of CMD word.
						configurestep = CFG_STEP_SET_FAULTRESET;
                        goto lblCfgStepSetFaultReset;
					case CFG_STEP_SET_FAULTRESET:
                        lblCfgStepSetFaultReset:
						if (mc->async_write_multiply_registers(7122, 3)) //Configure fault reset.
							{
							configurestep = CFG_STEP_INIT_SAVESETTINGS;
                            goto lfbCfgStepInitSaveSettings;
							}
						break;

					case CFG_STEP_INIT_SAVESETTINGS:
                        lfbCfgStepInitSaveSettings:
						mc->zero_output_buff();
						mc->set_station(0);
						mc->set_int2(0, 1);
						configurestep = CFG_STEP_SET_SAVESETTINGS;
                        goto lblCfgStepSetSaveSettings;
					case CFG_STEP_SET_SAVESETTINGS:
                        lblCfgStepSetSaveSettings:
						if (mc->async_write_multiply_registers(8001, 1)) //Save settings as Profile1
							{
							configurestep = CFG_STEP_END;
                            goto lblCfgStepEnd;
							}
						break;

					case CFG_STEP_END:
                        lblCfgStepEnd:
						configure = false;
						querystep = RUN_STEP_INIT_IOSCANNER;
                        goto lblRunStepInitIoScanner;
						break;
					default:
						configurestep = 0;
						break;
					}
				break;
			case RUN_STEP_INIT_IOSCANNER:
                lblRunStepInitIoScanner:
				mc->zero_output_buff(15);
				mc->set_station(255);
				mc->set_int2(2, cmd);
				mc->set_int2(3, (int)(frq_setpoint * 10));
				mc->set_int2(4, 0);
				mc->set_int2(5, 0);
				mc->set_int2(6, 0);
				mc->set_int2(7, 0);
                querystep = RUN_STEP_QUERY_IOSCANNER;
                goto lblRunIoScanner;
			case RUN_STEP_QUERY_IOSCANNER:
                lblRunIoScanner:
				start_addr = 1;
				if (type == TYPE_ATV630) start_addr = 0;
				commres = mc->async_read_write_multiply_registers(start_addr, 6, start_addr, 6);
				if (commres == 1)
					{
                    errorCount = 0;
					remote_state = mc->get_int2(0);
					rpm_value = mc->get_int2(1);
					frq_value = mc->get_int2(2) / 10.0f;
					frq_max = mc->get_int2(3) / 10.0f;
					if (frq_max < FRQ_MIN_SETTING) frq_max = FRQ_MIN_SETTING;
					if (frq_max > FRQ_MAX_SETTING) frq_max = FRQ_MAX_SETTING;
					amperage = mc->get_int2(4) / 10.0f;
					int newstate = remote_state & 0x006F;
                    int outState = 0;
					switch (newstate)
						{
						case 0x0040: //switch on disabled
                            outState = 0;
							break;
						case 0x0021: //ready to switch on
                            outState = 0;
							break;
						case 0x0023: //switched on
                            outState = 0;
							break;
						case 0x0027: //operation enabled
							reverse ? outState = 2 : outState = 1;
							break;
						case 0x0007: //quick stop active
                            outState = -1;
							break;
						default:
                            //G_LOG->error("Altivar unknown state %d", remote_state);
                            outState = -1;
							break;
						}
                    if ( outState < 0 )
                        {
                        if ( get_delta_millisec( errorTimer ) >= minErrorTimeout )
                            {
                            state = outState;
                            }
                        }
                    else
                        {
                        errorTimer = get_millisec( );
                        state = outState;
                        }
					querystep = RUN_STEP_INIT_END;
                    goto lblRunStepInitEnd;
					}
				if (commres == -1) //IO scanner not configured. Launch configuration.
					{
                    errorCount++;
                    if (errorCount >= errorCountToReconfigure)
                        {
                        configure = 1;
                        errorCount = 0;
                        querystep = RUN_STEP_CHECK_CONFIG;
                        goto lblRunStepCheckConfig;
                        }
                    goto lblRunStepInitIoScanner;
					}
				break;
			case RUN_STEP_INIT_END:
                lblRunStepInitEnd:
				querytimer = get_millisec();
				querystep = RUN_STEP_END;
                goto lblRunStepEnd;
				break;
			case RUN_STEP_END:
                lblRunStepEnd:
				if (get_delta_millisec(querytimer) > queryinterval)
					{
					querystep = RUN_STEP_CHECK_CONFIG;
					}
				break;
			default:
				querystep = RUN_STEP_CHECK_CONFIG;
				break;
			}


		int ar = mc->get_connected_state();
		if (ar != tcp_client::ACS_CONNECTED && get_delta_millisec(modbustimeout) > 5000L && !ismodbuserror)
			{
			state = -100;
			ismodbuserror = 1;
			}
		if (ar == tcp_client::ACS_CONNECTED && ismodbuserror)
			{
			ismodbuserror = 0;
			}
		if (ar == tcp_client::ACS_CONNECTED)
			{
			modbustimeout = get_millisec();
			}
		}

#endif

	}

	void altivar_node::Enable()
		{
		enabled = true;
		ismodbuserror = 0;
		}

	void altivar_node::Disable()
		{
		enabled = false;
		}

	void altivar_node::set_output_in_percent( float value )
		{
		frq_setpoint = value / 100.0f * frq_max;
		}

	float altivar_node::get_output_in_percent( )
		{
		return frq_setpoint / frq_max * 100.0f;
		}
