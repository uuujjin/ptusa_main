#include "operation_mngr_tests.h"

using namespace ::testing;

const int MAX_STR_SIZE = 500;

TEST( action, print )
	{
	action a1( "test" );
	testing::internal::CaptureStdout();
	a1.print();
	auto output = testing::internal::GetCapturedStdout();
	EXPECT_EQ( output, "" );

	virtual_valve v1( "TEST1_V1" );
	a1.add_dev( &v1 );
	testing::internal::CaptureStdout();
	a1.print();
	output = testing::internal::GetCapturedStdout();
	EXPECT_EQ( output, "test: { {TEST1_V1} } \n" );

	a1.add_dev( &v1, action::MAIN_GROUP, action::MAIN_SUBGROUP + 1 );
	testing::internal::CaptureStdout();
	a1.print();
	output = testing::internal::GetCapturedStdout();
	EXPECT_EQ( output, "test: { {TEST1_V1} {TEST1_V1} } \n" );

	a1.add_dev( &v1, action::MAIN_GROUP + 1, action::MAIN_SUBGROUP );
	testing::internal::CaptureStdout();
	a1.print();
	output = testing::internal::GetCapturedStdout();
	EXPECT_EQ( output, "test: { {TEST1_V1} {TEST1_V1} } { {TEST1_V1} } \n" );
	}

TEST( action, check )
	{
	action* a1 = new action( "test_action", 0 );
	const auto MAX_SIZE = 20;
	std::string buff( MAX_SIZE, '\0' );
	auto res = a1->check( &buff[ 0 ] );
	EXPECT_EQ( 0, res );
	EXPECT_STREQ( "", buff.c_str() );
	delete a1;
	}

TEST( action, set_bool_property )
	{
	action a1( "test_action", 0 );
	const auto MAX_SIZE = 20;
	std::string buff( MAX_SIZE, '\0' );
	auto res = a1.set_bool_property( &buff[ 0 ], false );
	EXPECT_EQ( 0, res );
	EXPECT_STREQ( "", buff.c_str() );
	}

TEST( action, check_devices )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	action a1( "test_action", 0 );
	virtual_valve v1( "TEST1_V1" );
	tech_dev_error err_v1( &v1 );
	v1.direct_set_state( valve::VALVE_STATE_EX::VX_ON_FB_ERR );
	virtual_valve v2( "TEST1_V2" );
	tech_dev_error err_v2( &v2 );
	v2.direct_set_state( valve::VALVE_STATE_EX::VX_ON_FB_ERR );
	virtual_valve v3( "TEST1_V3" );
	tech_dev_error err_v3( &v3 );
	v3.direct_set_state( valve::VALVE_STATE_EX::VX_ON_FB_ERR );
	
	a1.add_dev( &v1 );

	const auto MAX_SIZE = 20;
	std::string buff ( MAX_SIZE, '\0' );

	auto r = a1.check_devices( &buff[ 0 ], MAX_SIZE );
	EXPECT_NE( 0, r );
	EXPECT_STREQ( "'TEST1_V1'", buff.c_str() );

	a1.add_dev( &v2 );
	a1.add_dev( &v3 );
	r = a1.check_devices( &buff[ 0 ], MAX_SIZE );
	EXPECT_EQ( MAX_SIZE, r );
	EXPECT_STREQ( "'TEST1_V1', 'TES...", buff.c_str() );

	test_params_manager::removeObject();
	}

TEST( action, is_empty )
	{
	action a1( "empty_action", 0 );
	EXPECT_EQ( true, a1.is_empty() );

	virtual_valve v1( "TEST1_V1" );
	a1.add_dev( &v1 );
	EXPECT_EQ( false, a1.is_empty() );
	}


TEST( open_seat_action, evaluate )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	saved_params_u_int_4& par = PAC_info::get_instance()->par;
	const int FLIP_INTERVAL_MS = 5;
	const int FLIP_DURATION_MS = 2;
	par[ PAC_info::P_MIX_FLIP_UPPER_TIME ] = FLIP_DURATION_MS;
	par[ PAC_info::P_MIX_FLIP_LOWER_TIME ] = FLIP_DURATION_MS;

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	DO1 test_DO1( "test_DO1", device::DEVICE_TYPE::DT_DO, device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	DO1 test_DO2( "test_DO2", device::DEVICE_TYPE::DT_DO, device::DEVICE_SUB_TYPE::DST_DO_VIRT );

	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
	auto operation = operation_mngr[ 0 ][ 1 ];
	operation->add_step( "Тестовый шаг", -1, -1 );
	auto operation_state = operation[ 0 ][ 1 ];
	auto step = operation_state[ 0 ][ 1 ];

	auto action = step[ 0 ][ step::ACTIONS::A_UPPER_SEATS_ON ];
	action->add_dev( &test_DO1, 0, valve::V_UPPER_SEAT );
	action->add_dev( &test_DO2, 0, valve::V_LOWER_SEAT );
	action->print();

	action->init();
	reinterpret_cast<open_seat_action*>( action )->set_wait_time( FLIP_INTERVAL_MS );

	action->evaluate();  // Wait
	EXPECT_EQ( false, test_DO1.is_active() );
	EXPECT_EQ( false, test_DO2.is_active() );

	sleep_ms( FLIP_INTERVAL_MS + 1 );
	action->evaluate();
	EXPECT_EQ( false, test_DO1.is_active() );
	EXPECT_EQ( false, test_DO2.is_active() );

	action->evaluate(); // Upper seats
	EXPECT_EQ( true, test_DO1.is_active() );
	EXPECT_EQ( false, test_DO2.is_active() );
	sleep_ms( FLIP_DURATION_MS + 1 );
	action->evaluate();
	EXPECT_EQ( false, test_DO1.is_active() );
	EXPECT_EQ( false, test_DO2.is_active() );

	action->evaluate();  // Wait
	EXPECT_EQ( false, test_DO1.is_active() );
	EXPECT_EQ( false, test_DO2.is_active() );
	sleep_ms( FLIP_INTERVAL_MS + 1 );
	action->evaluate();
	EXPECT_EQ( false, test_DO1.is_active() );
	EXPECT_EQ( false, test_DO2.is_active() );

	action->evaluate(); // Lower seats
	EXPECT_EQ( false, test_DO1.is_active() );
	EXPECT_EQ( true, test_DO2.is_active() );
	sleep_ms( FLIP_DURATION_MS + 1 );
	action->evaluate();
	EXPECT_EQ( false, test_DO1.is_active() );
	EXPECT_EQ( false, test_DO2.is_active() );

	action->finalize();

	test_params_manager::removeObject();
	}


TEST( off_action, evaluate )
	{
	saved_params_u_int_4& par = PAC_info::get_instance()->par;
	par[ PAC_info::P_V_OFF_DELAY_TIME ] = 0;

	valve::clear_switching_off_queue();

	virtual_valve V1( "V1" );
	virtual_valve V2( "V2" );
	virtual_valve V3( "V3" );
	virtual_device DO1( "DO1", device::DT_DO, device::DST_DO_VIRT );
	off_action action;
	action.add_dev( &V1 );
	action.add_dev( &V2 );
	action.add_dev( &DO1 );

	EXPECT_FALSE( V1.is_active() );
	EXPECT_FALSE( V2.is_active() );
	EXPECT_FALSE( V3.is_active() );
	EXPECT_FALSE( DO1.is_active() );

	V1.on();
	V2.on();
	V3.on();
	DO1.on();
	EXPECT_TRUE( V1.is_active() );
	EXPECT_TRUE( V2.is_active() );
	EXPECT_TRUE( V3.is_active() );
	EXPECT_TRUE( DO1.is_active() );
	action.init();
	sleep_ms( 1 );
	valve::evaluate();
	EXPECT_TRUE( V1.is_active() );
	EXPECT_TRUE( V2.is_active() );
	EXPECT_TRUE( V3.is_active() );
	EXPECT_TRUE( DO1.is_active() );

	V1.on();
	V2.on();
	V3.on();
	DO1.on();
	action.evaluate();
	sleep_ms( 1 );
	valve::evaluate();
	EXPECT_FALSE( V1.is_active() );
	EXPECT_FALSE( V2.is_active() );
	EXPECT_TRUE( V3.is_active() );
	EXPECT_FALSE( DO1.is_active() );
	
	action.finalize();
	EXPECT_FALSE( V1.is_active() );
	EXPECT_FALSE( V2.is_active() );
	EXPECT_TRUE( V3.is_active() );
	EXPECT_FALSE( DO1.is_active() );
	}


TEST( on_reverse_action, evaluate )
	{
	virtual_device M1( "DO1", device::DT_M, device::DST_M_VIRT );
	on_reverse_action action;
	action.add_dev( &M1 );

	EXPECT_FALSE( M1.is_active() );

	action.init();
	EXPECT_FALSE( M1.is_active() );

	M1.off();
	action.evaluate();
	EXPECT_TRUE( M1.is_active() );
	const auto REVERSE_STATE = 2;
	EXPECT_EQ( REVERSE_STATE, M1.get_state() );

	action.finalize();
	EXPECT_FALSE( M1.is_active() );
	}


TEST( step, get_name )
	{
	auto ST_NAME = "test_step";
	step st1( ST_NAME, 0 );
	EXPECT_STREQ( ST_NAME, st1.get_name() );
	}

TEST( step, is_active )
	{
	auto ST_NAME = "test_step";
	step st1( ST_NAME, 0 );
	EXPECT_FALSE( st1.is_active() );
	st1.init();
	EXPECT_TRUE( st1.is_active() );
	st1.finalize();
	EXPECT_FALSE( st1.is_active() );
	}


TEST( operation_state, print )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	lua_State* L = lua_open();
	ASSERT_EQ( 1, tolua_PAC_dev_open( L ) );
	G_LUA_MANAGER->set_Lua( L );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
	auto operation = ( *operation_mngr )[ 1 ];
	operation->add_step( "Тестовый шаг", -1, -1 );
	auto operation_state = ( *operation )[ 1 ];

	EXPECT_EQ( true, operation_state->is_empty() );
    testing::internal::CaptureStdout();
    operation_state->print();
    auto output = testing::internal::GetCapturedStdout();
    std::string str = "RUN\n"
        "    1     \"Тестовый шаг\" \n"
        " { }\n";
    EXPECT_EQ( str, output );

	auto step = ( *operation_state )[ -1 ];
	auto action = ( *step )[ step::ACTIONS::A_ON ];
	DO1 test_DO( "test_DO", device::DEVICE_TYPE::DT_DO, device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	action->add_dev( &test_DO );
	EXPECT_EQ( false, operation_state->is_empty() );
    testing::internal::CaptureStdout();
    operation_state->print();
    output = testing::internal::GetCapturedStdout();
    str = "RUN\n"
        "    0     \"Шаг операции\" \n"
        "      Включать: { {test_DO} } \n"
        " { }\n"
        "    1     \"Тестовый шаг\" \n"
        " { }\n";
    EXPECT_EQ( str, output );

	G_LUA_MANAGER->free_Lua();
	test_params_manager::removeObject();
	}

TEST( operation_state, is_empty )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	lua_State* L = lua_open();
	ASSERT_EQ( 1, tolua_PAC_dev_open( L ) );
	G_LUA_MANAGER->set_Lua( L );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
	auto operation = ( *operation_mngr )[ 1 ];
	operation->add_step( "Тестовый шаг", -1, -1 );
	auto operation_state = ( *operation )[ 1 ];	
	EXPECT_EQ( true, operation_state->is_empty() );

	auto step = ( *operation_state )[ -1 ];
	auto action = ( *step )[ step::ACTIONS::A_ON ];
	DO1 test_DO( "test_DO", device::DEVICE_TYPE::DT_DO, device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	action->add_dev( &test_DO );
	EXPECT_EQ( false, operation_state->is_empty() );

	G_LUA_MANAGER->free_Lua();
	test_params_manager::removeObject();
	}

TEST( operation_state, check_devices )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );


	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
	auto test_op = ( *operation_mngr )[ 1 ];
	test_op->add_step( "Тестовый шаг", -1, -1 );
	auto operation_run_state = ( *test_op )[ operation::RUN ];
	auto additional_step_in_run = operation_run_state->add_step( "Step_#2", -1, -1 );
	auto a1 = reinterpret_cast<on_action*>(
		( *additional_step_in_run )[ step::ACTIONS::A_ON ] );

	virtual_valve v1( "TEST1_V1" );
	tech_dev_error err_v1( &v1 );	
	a1->add_dev( &v1 );
	const auto MAX_SIZE = 20;
	std::string buff( MAX_SIZE, '\0' );
	auto ADDITIONAL_STEP = 2;
	test_op->start();
	test_op->to_step( ADDITIONAL_STEP );
	v1.direct_set_state( valve::VALVE_STATE_EX::VX_ON_FB_ERR );
	auto r = operation_run_state->check_devices( &buff[ 0 ], MAX_SIZE );
	EXPECT_NE( 0, r );
	EXPECT_STREQ( "'TEST1_V1'", buff.c_str() );


	test_params_manager::removeObject();
	}


TEST( operation, operator_at )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	lua_State* L = lua_open();
	ASSERT_EQ( 1, tolua_PAC_dev_open( L ) );
	G_LUA_MANAGER->set_Lua( L );


	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	auto test_op = test_tank.get_modes_manager()->add_operation( "Test operation" );	
	auto stub = ( *test_op )[ operation::STATES_MAX ];
	ASSERT_STREQ( "заглушка", stub->get_name() );


	G_LUA_MANAGER->free_Lua();
	test_params_manager::removeObject();
	}

TEST( operation, start )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	lua_State* L = lua_open();
	ASSERT_EQ( 1, tolua_PAC_dev_open( L ) );
	G_LUA_MANAGER->set_Lua( L );


	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	auto test_op = test_tank.get_modes_manager()->add_operation( "Test operation" );
	
	EXPECT_EQ( -1, test_op->get_run_step() );

	EXPECT_EQ( 0, test_op->get_run_steps_count() );

	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->pause();
	EXPECT_EQ( operation::PAUSE, test_op->get_state() );
	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->stop();
	EXPECT_EQ( operation::STOP, test_op->get_state() );
	test_op->start();
	EXPECT_EQ( operation::STOP, test_op->get_state() );
	test_op->finalize();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	test_op->switch_off(); //Операция уже в "IDLE", ничего не должно измениться.
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	G_LUA_MANAGER->free_Lua();
	test_params_manager::removeObject();
	}

TEST( operation, turn_off_active_step )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	lua_State* L = lua_open();
	ASSERT_EQ( 1, tolua_PAC_dev_open( L ) );
	G_LUA_MANAGER->set_Lua( L );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	auto test_op = test_tank.get_modes_manager()->add_operation( "Test operation" );
	test_op->add_step( "Init", -1, -1 );


	auto const MAIN_STEP = 1;
	EXPECT_EQ( 0, test_op->active_step() );
	test_op->start( MAIN_STEP );
	EXPECT_EQ( MAIN_STEP, test_op->active_step() );
	test_op->turn_off_active_step();
	EXPECT_EQ( 0, test_op->active_step() );


	G_LUA_MANAGER->free_Lua();
	test_params_manager::removeObject();
	}

TEST( operation, on_extra_step )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	lua_State* L = lua_open();
	ASSERT_EQ( 1, tolua_PAC_dev_open( L ) );
	G_LUA_MANAGER->set_Lua( L );


	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	auto test_op = test_tank.get_modes_manager()->add_operation( "Test operation" );
	test_op->add_step( "Init", -1, -1 );
	test_op->add_step( "Process #1", -1, -1 );
	test_op->add_step( "Process #2", -1, -1 );

	auto const MAIN_STEP = 1;
	test_op->start( MAIN_STEP );
	auto const EXTRA_STEP = 2;
	auto const ANOTHER_EXTRA_STEP = 3;
	EXPECT_FALSE( test_op->is_active_run_extra_step( EXTRA_STEP ) );
	EXPECT_FALSE( test_op->is_active_run_extra_step( ANOTHER_EXTRA_STEP ) );
	test_op->on_extra_step( EXTRA_STEP );
	EXPECT_TRUE( test_op->is_active_run_extra_step( EXTRA_STEP ) );
	EXPECT_FALSE( test_op->is_active_run_extra_step( ANOTHER_EXTRA_STEP ) );
	test_op->switch_active_extra_step( EXTRA_STEP, ANOTHER_EXTRA_STEP );
	EXPECT_FALSE( test_op->is_active_run_extra_step( EXTRA_STEP ) );
	EXPECT_TRUE( test_op->is_active_run_extra_step( ANOTHER_EXTRA_STEP ) );
	test_op->off_extra_step( ANOTHER_EXTRA_STEP );
	EXPECT_FALSE( test_op->is_active_run_extra_step( EXTRA_STEP ) );
	EXPECT_FALSE( test_op->is_active_run_extra_step( ANOTHER_EXTRA_STEP ) );

	//Время выполнения активного шага должно быть большее нуля.
	sleep_ms( 1 );
	EXPECT_GT( test_op->active_step_evaluation_time(), 0 );
	//Задание времения выполнения активного шага должно быть равно нулю.
	EXPECT_EQ( 0, test_op->get_active_step_set_time() );

	test_op->on_extra_step( EXTRA_STEP );
	test_op->on_extra_step( ANOTHER_EXTRA_STEP );
	EXPECT_TRUE( test_op->is_active_run_extra_step( EXTRA_STEP ) );
	EXPECT_TRUE( test_op->is_active_run_extra_step( ANOTHER_EXTRA_STEP ) );
	test_op->finalize();
	EXPECT_FALSE( test_op->is_active_run_extra_step( EXTRA_STEP ) );
	EXPECT_FALSE( test_op->is_active_run_extra_step( ANOTHER_EXTRA_STEP ) );
	EXPECT_FALSE( test_op->is_active_extra_step( EXTRA_STEP ) );
	EXPECT_FALSE( test_op->is_active_extra_step( ANOTHER_EXTRA_STEP ) );
	

	G_LUA_MANAGER->free_Lua();
	test_params_manager::removeObject();
	}

TEST( operation, evaluate )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	lua_State* L = lua_open();
	ASSERT_EQ( 1, tolua_PAC_dev_open( L ) );
	G_LUA_MANAGER->set_Lua( L );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	auto test_op = test_tank.get_modes_manager()->add_operation( "Test operation" );

	//Корректный переход от выполнения к паузе и опять к выполнению для методов
	//операции когда в паузе нет шагов.
	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->evaluate();
	test_op->pause();
	test_op->evaluate();
	EXPECT_EQ( operation::PAUSE, test_op->get_state() );
	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->finalize();

	test_op->print( "" );

	test_op->add_step( "Init", 2, -1 );
	test_op->add_step( "Process #1", 3, -1 );
	test_op->add_step( "Process #2", 2, -1 );
	
	test_op->add_step( "Safe stop #1", 2, -1, operation::PAUSE );
	test_op->add_step( "Safe stop #2", 3, -1, operation::PAUSE );
	test_op->add_step( "Idle", -1, -1, operation::PAUSE );
	
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	test_op->pause();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	test_op->stop();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	test_op->finalize();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
		
	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->evaluate();
	test_op->pause();
	EXPECT_EQ( operation::PAUSE, test_op->get_state() );
	test_op->evaluate();
	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->evaluate();
	test_op->stop();
	EXPECT_EQ( operation::STOP, test_op->get_state() );
	test_op->evaluate();
	test_op->finalize();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	test_op->start();
	EXPECT_EQ( operation::RUN, test_op->get_state() );
	test_op->evaluate();
	test_op->pause();
	EXPECT_EQ( operation::PAUSE, test_op->get_state() );
	test_op->evaluate();
	test_op->stop();
	EXPECT_EQ( operation::STOP, test_op->get_state() );
	test_op->evaluate();
	test_op->finalize();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );


	//Корректный переход от выполнения к паузе и опять к выполнению для методов
	//операции.
	test_op->start();
	test_op->evaluate();
	test_op->to_next_step();
	test_op->evaluate();
	test_op->to_next_step();
	test_op->evaluate();
	EXPECT_EQ( 3, test_op->active_step() );
	EXPECT_EQ( 3, test_op->get_run_active_step() );

	test_op->pause();
	EXPECT_EQ( 1, test_op->active_step() );
	test_op->to_next_step();
	EXPECT_EQ( 2, test_op->active_step() );
	test_op->evaluate();
	EXPECT_EQ( 2, test_op->active_step() );

	test_op->start();
	EXPECT_EQ( 3, test_op->active_step() );
	test_op->evaluate();
	EXPECT_EQ( 3, test_op->active_step() );

	test_op->switch_off();

	//Корректное автовключение/автоотключение.	
	auto operation_idle_state = ( *test_op )[ operation::IDLE ];
	auto operation_run_state = ( *test_op )[ operation::RUN ];
	auto operation_pause_state = ( *test_op )[ operation::PAUSE ];

	auto main_step_in_idle = ( *operation_idle_state )[ -1 ];
	auto main_step_in_run = ( *operation_run_state )[ -1 ];
	auto main_step_in_pause = ( *operation_pause_state )[ -1 ];

	int next = 0;
	auto is_goto_next_state = operation_idle_state->is_goto_next_state( next );
	EXPECT_EQ( false, is_goto_next_state );			//Empty if_action_in_idle.
	EXPECT_EQ( -1, next );

	auto if_action_in_idle = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_idle )[ step::ACTIONS::A_JUMP_IF ] );
	DI1 test_DI_one( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	if_action_in_idle->add_dev( &test_DI_one );
	
	auto if_action_in_run = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_run )[ step::ACTIONS::A_JUMP_IF ] );
	if_action_in_run->add_dev( &test_DI_one, 0, 1 );

	//По умолчанию все сигналы неактивны, операция не должна включиться.
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );	

	//Сигнал активен, операция должна включиться.
	test_DI_one.on();
	test_op->evaluate();
	EXPECT_EQ( operation::RUN, test_op->get_state() );

	//Сигнал не активен, операция должна отключиться.
	test_DI_one.off();
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	DI1 test_DI_two( "test_DI2", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	auto main_jump_if_action = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_run )[ step::ACTIONS::A_REQUIRED_FB ] );
	main_jump_if_action->add_dev( &test_DI_two );

	//Сигнал активен, но операция не должна включиться, так как нет требуемого
	//сигнала.
	const auto WARN_TIME = 1;
	const auto WAIT_TIME = 2 * WARN_TIME;
	G_PAC_INFO()->par[ PAC_info::P_AUTO_OPERATION_WAIT_TIME ] = WAIT_TIME;
	G_PAC_INFO()->par[ PAC_info::P_AUTO_OPERATION_WARN_TIME ] = WARN_TIME;
	test_DI_one.on();
	test_DI_two.off();
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	sleep_ms( WARN_TIME );
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	sleep_ms( WAIT_TIME );
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	//Сигнал активен, но операция не должна включиться, так как автовключение 
	//было отключено.
	test_DI_one.on();
	test_DI_two.on();
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	//Сигнал активен, операция должна включиться, так как автовключение 
	//было опять включено.
	test_DI_one.off();
	test_DI_two.on();
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );
	test_DI_one.on();
	test_DI_two.on();
	test_op->evaluate();
	EXPECT_EQ( operation::RUN, test_op->get_state() );

	//Сигнал не активен, операция должна отключиться.
	test_DI_one.off();
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	//Сигнал активен, но операция не должна включиться, так как нет требуемого
	//сигнала.
	test_DI_one.on();
	test_DI_two.off();
	test_op->evaluate();
	EXPECT_EQ( operation::IDLE, test_op->get_state() );

	//Сигнал активен, а операция должна включиться, так как появился требуемый
	//сигнал.
	sleep_ms( WAIT_TIME );
	test_DI_one.on();
	test_DI_two.on();
	test_op->evaluate();
	EXPECT_EQ( operation::RUN, test_op->get_state() );

	//В паузе переходы к новым состояниям не осуществляются.
	test_op->switch_off();
	DI1 test_DI_three( "test_DI3", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	auto if_action_in_pause = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_pause )[ step::ACTIONS::A_JUMP_IF ] );
	if_action_in_pause->add_dev( &test_DI_one );
	test_op->start();
	test_op->pause();
	test_DI_three.on();
	test_op->evaluate();
	EXPECT_EQ( operation::PAUSE, test_op->get_state() );

	test_op->finalize();

	DO1 test_DO_one( "test_DO1", device::DEVICE_TYPE::DT_DO,
		device::DEVICE_SUB_TYPE::DST_DO_VIRT );


	//При наличии описания состояния "Starting" переходим к нему.
	auto operation_starting_state = ( *test_op )[ operation::STARTING ];
	auto main_step_in_starting = ( *operation_starting_state )[ -1 ];
	auto on_action_in_starting = ( *main_step_in_starting )[ step::ACTIONS::A_ON ];
	on_action_in_starting->add_dev( &test_DO_one );

	test_op->start();
	EXPECT_EQ( operation::STARTING, test_op->get_state() );

	//При наличии перехода по условию, переходим к "Run".
	auto if_action_in_starting = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_starting )[ step::ACTIONS::A_JUMP_IF ] );
	if_action_in_starting->add_dev( &test_DI_one );
	test_DI_one.on();
	test_op->evaluate();
	EXPECT_EQ( operation::RUN, test_op->get_state() );

	on_action_in_starting->clear_dev();
	if_action_in_starting->clear_dev();
	test_op->finalize();


    //При наличии описания состояния "Pausing" переходим к нему.
    auto operation_pausing_state = ( *test_op )[ operation::PAUSING ];
    auto main_step_in_pausing = ( *operation_pausing_state )[ -1 ];
	auto on_action_in_pausing = ( *main_step_in_pausing )[ step::ACTIONS::A_ON ];
	on_action_in_pausing->add_dev( &test_DO_one );

    test_op->start();
    test_op->pause();
    EXPECT_EQ( operation::PAUSING, test_op->get_state() );

	//При наличии перехода по условию, переходим к "Pause".
	auto if_action_in_pausing = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_pausing )[ step::ACTIONS::A_JUMP_IF ] );
	if_action_in_pausing->add_dev( &test_DI_one );
	test_DI_one.on();
	test_op->evaluate();
	EXPECT_EQ( operation::PAUSE, test_op->get_state() );

	on_action_in_pausing->clear_dev();
	if_action_in_pausing->clear_dev();
	test_op->finalize();


	//При наличии описания состояния "Unpausing" переходим к нему.
	auto operation_unpausing_state = ( *test_op )[ operation::UNPAUSING ];
	auto main_step_in_unpausing = ( *operation_unpausing_state )[ -1 ];
	auto on_action_in_unpausing = ( *main_step_in_unpausing )[ step::ACTIONS::A_ON ];
	on_action_in_unpausing->add_dev( &test_DO_one );

	test_op->start();
	test_op->pause();
	test_op->start();
	EXPECT_EQ( operation::UNPAUSING, test_op->get_state() );

	//При наличии перехода по условию, переходим к "Run".
	auto if_action_in_unpausing = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_unpausing )[ step::ACTIONS::A_JUMP_IF ] );
	if_action_in_unpausing->add_dev( &test_DI_one );
	test_DI_one.on();
	test_op->evaluate();
	EXPECT_EQ( operation::RUN, test_op->get_state() );

	on_action_in_unpausing->clear_dev();
	if_action_in_unpausing->clear_dev();


    //При наличии описания состояния "Stopping" переходим к нему.
    auto operation_stopping_state = ( *test_op )[ operation::STOPPING ];
    auto main_step_in_stopping = ( *operation_stopping_state )[ -1 ];
	auto on_action_in_stopping = ( *main_step_in_stopping )[ step::ACTIONS::A_ON ];
	on_action_in_stopping->add_dev( &test_DO_one );

    test_op->stop();
    EXPECT_EQ( operation::STOPPING, test_op->get_state() );

	//При наличии перехода по условию, переходим к "Stop".
	auto if_action_in_stopping = reinterpret_cast<jump_if_action*>
		( ( *main_step_in_stopping )[ step::ACTIONS::A_JUMP_IF ] );
	if_action_in_stopping->add_dev( &test_DI_one );
	test_DI_one.on();
	test_op->evaluate();
	EXPECT_EQ( operation::STOP, test_op->get_state() );

	on_action_in_unpausing->clear_dev();
	if_action_in_unpausing->clear_dev();

	test_op->finalize();


    G_LUA_MANAGER->free_Lua();
    test_params_manager::removeObject();
	}

/*
	TEST METHOD DEFENITION:
	void check()
*/

TEST( AI_AO_action, check )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	analog_output test_AO( "test_AO1" );
	analog_input test_AI( "test_AI1" );

	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
    auto operation = ( *operation_mngr )[ 1 ];
    auto operation_state = ( *operation )[ 1 ];
	auto step = ( *operation_state )[ 1 ];

	auto action = ( *step )[ step::ACTIONS::A_AI_AO ];	
	char msg[ 1024 ];

	EXPECT_EQ( 0, action->check( msg ) );

	action->add_dev( &test_AO );
	action->add_dev( &test_AI );
	EXPECT_NE( 0, action->check( msg ) );

	action->clear_dev();
	action->add_dev( &test_AI );
	action->add_dev( &test_AO );
	EXPECT_EQ( 0, action->check( msg ) );

	test_params_manager::removeObject();
	}


TEST( checked_devices_action, finalize )
	{
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO, device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	auto action = checked_devices_action();
	action.add_dev( &test_DO );
	
	test_DO.on();
	action.init();
	action.evaluate();
	EXPECT_EQ( 1, test_DO.get_state() );
	
	action.finalize();
	EXPECT_EQ( 1, test_DO.get_state() );
	}


TEST( delay_on_action, evaluate )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO, device::DEVICE_SUB_TYPE::DST_DO_VIRT );

	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
	auto operation = ( *operation_mngr )[ 1 ];
	auto operation_state = ( *operation )[ 1 ];
	auto step = ( *operation_state )[ -1 ];

	auto action = ( *step )[ step::ACTIONS::A_DELAY_ON ];

	action->add_dev( &test_DO );
	const int DELAY_TIME_MS = 10;
	const int DELAY_PARAM_IDX = 2;
	test_tank.par_float[ DELAY_PARAM_IDX ] = DELAY_TIME_MS;
	action->set_param_idx( 0, DELAY_PARAM_IDX );
	action->init();

	action->evaluate();
	EXPECT_EQ( 0, test_DO.get_state() );

	sleep_ms( DELAY_TIME_MS + 1 );
	action->evaluate();
	EXPECT_EQ( 1, test_DO.get_state() );

	action->finalize();
	EXPECT_EQ( 0, test_DO.get_state() );

	test_params_manager::removeObject();
	}

TEST( delay_off_action, evaluate )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO, device::DEVICE_SUB_TYPE::DST_DO_VIRT );

	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
	auto operation = ( *operation_mngr )[ 1 ];
	auto operation_state = ( *operation )[ 1 ];
	auto step = ( *operation_state )[ -1 ];

	auto action = ( *step )[ step::ACTIONS::A_DELAY_OFF ];

	action->add_dev( &test_DO );
	const int DELAY_TIME_MS = 10;
	const int DELAY_PARAM_IDX = 2;
	test_tank.par_float[ DELAY_PARAM_IDX ] = DELAY_TIME_MS;
	action->set_param_idx( 0, DELAY_PARAM_IDX );
	action->init();

	action->evaluate();
	EXPECT_EQ( 1, test_DO.get_state() );

	sleep_ms( DELAY_TIME_MS + 1 );
	action->evaluate();
	EXPECT_EQ( 0, test_DO.get_state() );

	action->finalize();
	EXPECT_EQ( 0, test_DO.get_state() );

	test_params_manager::removeObject();
	}


TEST( required_DI_action, check )
	{
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	test_DI.set_descr( "Test DI" );
	auto action = required_DI_action();
	action.add_dev( &test_DI );

	std::string msg( MAX_STR_SIZE, '\0');
	auto res = action.check( &msg[ 0 ] );
	EXPECT_EQ( 1, res );
	EXPECT_STREQ( "нет сигнала 'test_DI1 (Test DI)'", msg.c_str() );

	test_DI.on();
	res = action.check( &msg[ 0 ] );
	EXPECT_EQ( 0, res );
	EXPECT_STREQ( "", msg.c_str() );
	}

TEST( required_DI_action, finalize )
	{
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	auto action = required_DI_action();
	action.add_dev( &test_DI );

	test_DI.on();
	action.init();
	action.evaluate();
	EXPECT_EQ( 1, test_DI.get_state() );

	action.finalize();
	EXPECT_EQ( 1, test_DI.get_state() );
	}


TEST( DI_DO_action, check )
	{
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO,
		device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );

	test_DO.set_descr( "Test DO" );
	auto action = DI_DO_action();
	action.add_dev( &test_DO );

	std::string msg( MAX_STR_SIZE, '\0' );
	auto res = action.check( &msg[ 0 ] );
	EXPECT_EQ( 1, res );
	EXPECT_STREQ( "в поле 'Группы DI->DO's' устройство 'test_DO1 (Test DO)'"
		" не является входным сигналом (DI, SB, GS, LS, FS)", msg.c_str() );

	action.clear_dev();
	action.add_dev( &test_DI );
	action.add_dev( &test_DO );
	res = action.check( &msg[ 0 ] );
	EXPECT_EQ( 0, res );
	EXPECT_STREQ( "", msg.c_str() );
	}

TEST( DI_DO_action, evaluate )
	{
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO,
		device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	test_DO.set_descr( "Test DO" );
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );

	auto action = DI_DO_action();
	action.add_dev( &test_DI );
	action.add_dev( &test_DO );

	std::string msg( MAX_STR_SIZE, '\0' );
	auto res = action.check( &msg[ 0 ] );
	EXPECT_EQ( 0, res );
	EXPECT_STREQ( "", msg.c_str() );

	EXPECT_FALSE( test_DI.is_active() );
	action.evaluate();
	EXPECT_FALSE( test_DO.is_active() );

	test_DI.set_cmd( "ST", 0, 1 );
	EXPECT_TRUE( test_DI.is_active() );
	action.evaluate();
	EXPECT_TRUE( test_DO.is_active() );

	test_DI.set_cmd( "ST", 0, 0.f );
	EXPECT_FALSE( test_DI.is_active() );
	action.evaluate();
	EXPECT_FALSE( test_DO.is_active() );
	}

TEST( DI_DO_action, finalize )
	{
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO,
		device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	auto action = DI_DO_action();
	action.add_dev( &test_DI, action::MAIN_GROUP, 1 );
	action.add_dev( &test_DO, action::MAIN_GROUP, 1 );

	test_DI.on();
	action.init();
	action.evaluate();
	EXPECT_EQ( 1, test_DI.get_state() );
	EXPECT_EQ( 1, test_DO.get_state() );

	action.finalize();
	EXPECT_EQ( 1, test_DI.get_state() );
	EXPECT_EQ( 0, test_DO.get_state() );
	}


TEST( inverted_DI_DO_action, evaluate )
	{
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO,
		device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	test_DO.set_descr( "Test DO" );
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );

	auto action = inverted_DI_DO_action();
	action.add_dev( &test_DI );
	action.add_dev( &test_DO );

	std::string msg( MAX_STR_SIZE, '\0' );
	auto res = action.check( &msg[ 0 ] );
	EXPECT_EQ( 0, res );
	EXPECT_STREQ( "", msg.c_str() );

	EXPECT_FALSE( test_DI.is_active() );
	action.evaluate();
	EXPECT_TRUE( test_DO.is_active() );

	test_DI.set_cmd( "ST", 0, 1 );
	EXPECT_TRUE( test_DI.is_active() );
	action.evaluate();
	EXPECT_FALSE( test_DO.is_active() );

	test_DI.set_cmd( "ST", 0, 0.f );
	EXPECT_FALSE( test_DI.is_active() );
	action.evaluate();
	EXPECT_TRUE( test_DO.is_active() );
	}


TEST( AI_AO_action, finalize )
	{
	DO1 test_AO( "test_AO1", device::DEVICE_TYPE::DT_AO,
		device::DEVICE_SUB_TYPE::DST_AO_VIRT );
	DI1 test_AI( "test_AI1", device::DEVICE_TYPE::DT_AI,
		device::DEVICE_SUB_TYPE::DST_AI_VIRT, 0 );
	auto action = AI_AO_action();
	action.add_dev( &test_AI, action::MAIN_GROUP, 1 );
	action.add_dev( &test_AO, action::MAIN_GROUP, 1 );

	const int VALUE = 50;
	test_AI.set_value( VALUE );
	action.init();
	action.evaluate();
	EXPECT_EQ( VALUE, test_AI.get_value() );
	EXPECT_EQ( VALUE, test_AO.get_value() );

	action.finalize();
	EXPECT_EQ( VALUE, test_AI.get_value() );
	EXPECT_EQ( 0, test_AO.get_value() );
	}

TEST( wash_action, finalize )
	{
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO,
		device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );

	virtual_motor test_M1( "M1" );
	virtual_motor test_M2( "M2" );

	auto action = wash_action();
	action.add_dev( &test_DI, action::MAIN_GROUP, 0 );
	action.add_dev( &test_DO, action::MAIN_GROUP, 1 );
	action.add_dev( &test_M1, action::MAIN_GROUP, 2 );
	action.add_dev( &test_M2, action::MAIN_GROUP, 3 );

	test_DI.on();
	action.init();
	action.evaluate();
	EXPECT_EQ( 1, test_DI.get_state() );
	EXPECT_EQ( 1, test_DO.get_state() );
	EXPECT_EQ( 1, test_M1.get_state() );
	EXPECT_EQ( 2, test_M2.get_state() );

	action.finalize();
	EXPECT_EQ( 1, test_DI.get_state() );
	EXPECT_EQ( 0, test_DO.get_state() );
	EXPECT_EQ( 0, test_M1.get_state() );
	EXPECT_EQ( 0, test_M2.get_state() );
	}

TEST( wash_action, print )
	{
	DO1 test_DO( "test_DO1", device::DEVICE_TYPE::DT_DO,
		device::DEVICE_SUB_TYPE::DST_DO_VIRT );
	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );

	virtual_motor test_M1( "M1" );
	virtual_motor test_M2( "M2" );

	auto action = wash_action();

	testing::internal::CaptureStdout();
	action.print();
	auto output = testing::internal::GetCapturedStdout();
	EXPECT_EQ( "", output);

	action.add_dev( &test_DI, action::MAIN_GROUP, action::MAIN_SUBGROUP );
	action.add_dev( &test_DO, action::MAIN_GROUP, action::MAIN_SUBGROUP + 1 );
	action.add_dev( &test_M1, action::MAIN_GROUP, action::MAIN_SUBGROUP + 2 );
	action.add_dev( &test_M2, action::MAIN_GROUP, action::MAIN_SUBGROUP + 3 );
	action.set_param_idx( 0, 1 );

	testing::internal::CaptureStdout();
	action.print();
	output = testing::internal::GetCapturedStdout();
	EXPECT_EQ( "Устройства DI's DO's DEV's R_DEV's AI:"
		" { {test_DI1} {test_DO1} {M1} {M2} {} } ; FREQ_PARAM {1}\n", output );
	}


TEST( enable_step_by_signal, set_bool_property )
	{
	auto action = enable_step_by_signal();

	EXPECT_EQ( 0, action.set_bool_property( "should_turn_off", false ) );
	EXPECT_EQ( 1, action.set_bool_property( "wrong_name", false ) );
	}

TEST( enable_step_by_signal, should_turn_off )
    {
	auto action = enable_step_by_signal();

	EXPECT_EQ( true, action.should_turn_off() );	//Empty device list.

	DI1 test_DI( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	action.add_dev( &test_DI );
	EXPECT_EQ( true, action.should_turn_off() );	//Default flag value.

	action.set_bool_property( "should_turn_off", false );
	EXPECT_EQ( false, action.should_turn_off() );	//Flag was set to false.

	action.set_bool_property( "should_turn_off", true );
	EXPECT_EQ( true, action.should_turn_off() );	//Flag was set to true.
    }

TEST( enable_step_by_signal, is_any_group_active )
	{
	auto action = enable_step_by_signal();

	EXPECT_FALSE( action.is_any_group_active() );	//Empty device list.

	DI1 test_DI1( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	action.add_dev( &test_DI1 );
	EXPECT_FALSE( action.is_any_group_active() );	//No active group.

	test_DI1.set_cmd( "ST", 0, 1 );
	EXPECT_TRUE( action.is_any_group_active() );	//One active group.

	DI1 test_DI2( "test_DI2", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	action.add_dev( &test_DI2 );
	EXPECT_FALSE( action.is_any_group_active() );	//No active group.
	
	DI1 test_DI3( "test_DI3", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	action.add_dev( &test_DI3, action::MAIN_GROUP, action::MAIN_SUBGROUP + 1 );
	test_DI3.set_cmd( "ST", 0, 1 );
	EXPECT_TRUE( action.is_any_group_active() );	//One active group.
	}


TEST( jump_if_action, is_goto_next_step )
	{
	char* res = 0;
	mock_params_manager* par_mock = new mock_params_manager();
	test_params_manager::replaceEntity( par_mock );

	EXPECT_CALL( *par_mock, init( _ ) );
	EXPECT_CALL( *par_mock, final_init( _, _, _ ) );
	EXPECT_CALL( *par_mock, get_params_data( _, _ ) )
		.Times( AtLeast( 2 ) )
		.WillRepeatedly( Return( res ) );

	par_mock->init( 0 );
	par_mock->final_init( 0, 0, 0 );

	tech_object test_tank( "Танк1", 1, 1, "T", 10, 10, 10, 10, 10, 10 );

	test_tank.get_modes_manager()->add_operation( "Тестовая операция" );
	auto operation_mngr = test_tank.get_modes_manager();
	auto operation = ( *operation_mngr )[ 1 ];
	operation->add_step( "Тестовый шаг 1", -1, -1 );
	operation->add_step( "Тестовый шаг 2", -1, -1 );
	auto operation_state = operation[ 0 ][ 1 ];
	auto step = operation_state[ 0 ][ 1 ];

	operation->start();
	operation->evaluate();

	auto action = reinterpret_cast<jump_if_action*>
		( ( *step )[ step::ACTIONS::A_JUMP_IF ] );

	int next_step = 0;
	auto is_goto_next_step = action->is_jump( next_step );
	EXPECT_EQ( false, is_goto_next_step );			//Empty if_action_in_idle.
	EXPECT_EQ( -1, next_step );

	DI1 test_DI_one( "test_DI1", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	action->add_dev( &test_DI_one, 0, 0 );
	DI1 test_DI_two( "test_DI2", device::DEVICE_TYPE::DT_DI,
		device::DEVICE_SUB_TYPE::DST_DI_VIRT, 0 );
	action->add_dev( &test_DI_two, 0, 1 );
	valve_DO1 test_valve( "V3" );
	action->add_dev( &test_valve, 1, 0 );
	const int SET_NEXT_STEP = 2;
	EXPECT_EQ( 1, action->set_int_property( "no_exist", 0, SET_NEXT_STEP ) );
	EXPECT_EQ( 0, action->set_int_property( "next_step_n", 0, SET_NEXT_STEP ) );	

	EXPECT_EQ( -1, action->get_int_property( "no_exist", 0 ) );
	next_step = action->get_int_property( "next_step_n", 0 );
	EXPECT_EQ( SET_NEXT_STEP, next_step );

	//По умолчанию все сигналы неактивны, к новому шагу не должно быть
	//перехода.
	is_goto_next_step = action->is_jump( next_step );
	EXPECT_EQ( false, is_goto_next_step );
	EXPECT_EQ( SET_NEXT_STEP, next_step );

	//Устанавливаем сигналы, к новому шагу не должно быть перехода.
	test_DI_one.on();
	test_DI_two.on();
	test_valve.off();
	is_goto_next_step = action->is_jump( next_step );
	EXPECT_EQ( false, is_goto_next_step );
	EXPECT_EQ( SET_NEXT_STEP, next_step );

	//Устанавливаем сигналы, к новому шагу должен быть переход.
	test_DI_one.on();
	test_DI_two.off();
	test_valve.off();
	is_goto_next_step = action->is_jump( next_step );
	EXPECT_EQ( true, is_goto_next_step );
	EXPECT_EQ( SET_NEXT_STEP, next_step );

	//Устанавливаем сигналы, к новому шагу должен быть переход.
	test_DI_one.off();
	test_DI_two.off();
	test_valve.on();
	test_valve.set_cmd( "FB_ON_ST", 0 , 1 );
	is_goto_next_step = action->is_jump( next_step );
	EXPECT_EQ( true, is_goto_next_step );
	EXPECT_EQ( SET_NEXT_STEP, next_step );

	//Выполняем операцию, должен осуществиться переход к новому шагу.
	operation->evaluate();
	EXPECT_EQ( SET_NEXT_STEP, operation->get_run_active_step() );	
	

	//Проверяем переход по состоянию устройства "кнопка" ("SB").
	operation->switch_off();
	virtual_device test_SB1( "test_SB1", device::DT_SB, device::DST_SB_VIRT );
	action->clear_dev();
	action->add_dev( &test_SB1 );	

	//По умолчанию кнопка неактивна, к новому шагу не должно быть
	//перехода.
	is_goto_next_step = action->is_jump( next_step );
	EXPECT_FALSE( is_goto_next_step );
	EXPECT_EQ( SET_NEXT_STEP, next_step );

	//Устанавливаем состояние кнопки, к новому шагу должен быть переход.
	test_SB1.on();
	is_goto_next_step = action->is_jump( next_step );
	EXPECT_TRUE( is_goto_next_step );
	EXPECT_EQ( SET_NEXT_STEP, next_step );

	test_params_manager::removeObject();
	}
