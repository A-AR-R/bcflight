/*
 * BCFlight
 * Copyright (C) 2016 Adrien Aubry (drich)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
#include <list>
#include <fstream>
#include <algorithm>
#include <regex>
#include <string.h>
#include "Debug.h"
#include "Config.h"
#include <Accelerometer.h>
#include <Gyroscope.h>
#include <Magnetometer.h>

Config::Config( const std::string& filename, const std::string& settings_filename )
	: mFilename( filename )
	, mSettingsFilename( settings_filename )
	, L( nullptr )
{
	L = luaL_newstate();
	luaL_openlibs( L );

	Reload();
}


Config::~Config()
{
	lua_close(L);
}


std::string Config::ReadFile()
{
	std::string ret = "";
	std::ifstream file( mFilename );

	if ( file.is_open() ) {
		file.seekg( 0, file.end );
		int length = file.tellg();
		char* buf = new char[ length + 1 ];
		file.seekg( 0, file.beg );
		file.read( buf, length );
		buf[length] = 0;
		ret = buf;
		delete buf;
		file.close();
	}

	return ret;
}


void Config::WriteFile( const std::string& content )
{
	std::ofstream file( mFilename );

	if ( file.is_open() ) {
		file.write( content.c_str(), content.length() );
		file.close();
	}
}


std::string Config::string( const std::string& name, const std::string& def )
{
	gDebug() << "Config::string( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	std::string ret = lua_tolstring( L, -1, nullptr );
	Debug() << " => " << ret << "\n";
	return ret;
}


int Config::integer( const std::string& name, int def )
{
	gDebug() << "Config::integer( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	int ret = lua_tointeger( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


float Config::number( const std::string& name, float def )
{
	gDebug() << "Config::number( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	float ret = lua_tonumber( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


bool Config::boolean( const std::string& name, bool def )
{
	gDebug() << "Config::boolean( " << name << " )";

	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return def;
	}

	bool ret = lua_toboolean( L, -1 );
	Debug() << " => " << ret << "\n";
	return ret;
}


std::vector<int> Config::integerArray( const std::string& name )
{
	gDebug() << "Config::integerArray( " << name << " )";
	if ( LocateValue( name ) < 0 ) {
		Debug() << " => not found !\n";
		return std::vector<int>();
	}

	std::vector<int> ret;
	size_t len = lua_objlen( L, -1 );
	if ( len > 0 ) {
		for ( size_t i = 1; i <= len; i++ ) {
			lua_rawgeti( L, -1, i );
			int value = lua_tointeger( L, -1 );
			ret.emplace_back( value );
			lua_pop( L, 1 );
		}
	}
	lua_pop( L, 1 );
	Debug() << " => Ok\n";
	return ret;
}


int Config::LocateValue( const std::string& _name )
{
	const char* name = _name.c_str();

	if ( strchr( name, '.' ) == nullptr and strchr( name, '[' ) == nullptr ) {
		lua_getfield( L, LUA_GLOBALSINDEX, name );
		if ( lua_type( L, -1 ) == LUA_TNIL ) {
			return -1;
		}
	} else {
		char tmp[128];
		int i=0, j, k;
		bool in_table = false;
		for ( i = 0, j = 0, k = 0; name[i]; i++ ) {
			if ( name[i] == '.' or name[i] == '[' or name[i] == ']' ) {
				tmp[j] = 0;
				if ( strlen(tmp) == 0 ) {
					j = 0;
					k++;
					continue;
				}
				if ( name[i] == '[' ) {
					in_table = true;
				}
				if ( in_table and name[i] == ']' and lua_istable( L, -1 ) ) {
					if ( tmp[0] >= '0' and tmp[0] <= '9' ) {
						lua_rawgeti( L, -1, std::atoi(tmp) );
					} else {
						lua_getfield( L, -1, tmp );
					}
				} else {
					if ( k == 0 ) {
						lua_getfield( L, LUA_GLOBALSINDEX, tmp );
					} else {
						lua_getfield( L, -1, tmp );
					}
				}
				if ( lua_type( L, -1 ) == LUA_TNIL ) {
					return -1;
				}
				memset( tmp, 0, sizeof( tmp ) );
				j = 0;
				k++;
			} else {
				tmp[j] = name[i];
				j++;
			}
		}
		tmp[j] = 0;
		if ( tmp[0] != 0 ) {
			lua_getfield( L, -1, tmp );
			if ( lua_type( L, -1 ) == LUA_TNIL ) {
				return -1;
			}
		}
	}

	return 0;
}


int Config::ArrayLength( const std::string& name )
{
	if ( LocateValue( name ) < 0 ) {
		return -1;
	}

	int ret = -1;

	if ( lua_istable( L, -1 ) ) {
		ret = 0;
		size_t len = lua_objlen( L, -1 );
		if ( len > 0 ) {
			ret = len;
		} else {
			lua_pushnil( L );
			while( lua_next( L, -2 ) != 0 ) {
				ret++;
				lua_pop( L, 1 );
			}
		}
	}

	return ret;
}


void Config::DumpVariable( const std::string& name, int index, int indent )
{
	for ( int i = 0; i < indent; i++ ) {
		Debug() << "    ";
	}
	if ( name != "" ) {
		Debug() << name << " = ";
	}

	if ( indent == 0 ) {
		LocateValue( name );
	}

	if ( lua_isnil( L, index ) ) {
		Debug() << "nil";
	} else if ( lua_isnumber( L, index ) ) {
		Debug() << lua_tonumber( L, index );
	} else if ( lua_isboolean( L, index ) ) {
		Debug() << ( lua_toboolean( L, index ) ? "true" : "false" );
	} else if ( lua_isstring( L, index ) ) {
		Debug() << "\"" << lua_tostring( L, index ) << "\"";
	} else if ( lua_iscfunction( L, index ) ) {
		Debug() << "C-function()";
	} else if ( lua_isuserdata( L, index ) ) {
		Debug() << "__userdata__";
	} else if ( lua_istable( L, index ) ) {
		Debug() << "{\n";
		size_t len = lua_objlen( L, index );
		if ( len > 0 ) {
			for ( size_t i = 1; i <= len; i++ ) {
				lua_rawgeti( L, index, i );
				DumpVariable( "", -1, indent + 1 );
				lua_pop( L, 1 );
				Debug() << ",\n";
			}
		} else {
			lua_pushnil( L );
			while( lua_next( L, -2 ) != 0 ) {
				std::string key = lua_tostring( L, index-1 );
				if ( lua_isnumber( L, index - 1 ) ) {
					key = "[" + key + "]";
				}
				if ( key != "lens_shading" ) {
					DumpVariable( key, index, indent + 1 );
				} else {
					for ( int i = 0; i < indent + 1; i++ ) {
						Debug() << "    ";
					}
					std::cout << key << " = {...}";
				}
				lua_pop( L, 1 );
				Debug() << ",\n";
			}
		}
		for ( int i = 0; i < indent; i++ ) {
			Debug() << "    ";
		}
		Debug() << "}";
	} else {
		Debug() << "__unknown__";
	}

	if ( indent == 0 ) {
		Debug() << "\n";
	}
}


void Config::Execute( const std::string& code )
{
	luaL_dostring( L, code.c_str() );
}


static inline std::string& ltrim( std::string& s, const char* t = " \t\n\r\f\v" )
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}


static inline std::string& rtrim( std::string& s, const char* t = " \t\n\r\f\v" )
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}


static inline std::string& trim( std::string& s, const char* t = " \t\n\r\f\v" )
{
	return ltrim(rtrim(s, t), t);
}


void Config::Reload()
{
	luaL_dostring( L, "function Vector( x, y, z, w ) return { x = x, y = y, z = z, w = w } end" );
	luaL_dostring( L, "function Socket( params ) params.link_type = \"Socket\" ; return params end" );
	luaL_dostring( L, "function RF24( params ) params.link_type = \"nRF24L01\" ; return params end" );
	luaL_dostring( L, "function SX127x( params ) params.link_type = \"SX127x\" ; return params end" );
	luaL_dostring( L, "function MultiLink( params ) params.link_type = \"MultiLink\" ; return params end" );
	luaL_dostring( L, "function RawWifi( params ) params.link_type = \"RawWifi\" ; params.device = \"wlan0\" ; if params.blocking == nil then params.blocking = true end ; if params.retries == nil then params.retries = 2 end ; return params end" );
	luaL_dostring( L, "function Voltmeter( params ) params.sensor_type = \"Voltmeter\" ; return params end" );
	luaL_dostring( L, "function Buzzer( params ) params.type = \"Buzzer\" ; return params end" );
	luaL_dostring( L, ( "board = { type = \"" + std::string( BOARD ) + "\" }" ).c_str() );
	luaL_dostring( L, "frame = { motors = {} }" );
	luaL_dostring( L, "battery = {}" );
	luaL_dostring( L, "camera = {}" );
	luaL_dostring( L, "hud = {}" );
	luaL_dostring( L, "microphone = {}" );
	luaL_dostring( L, "controller = {}" );
	luaL_dostring( L, "stabilizer = { loop_time = 2000 }" );
	luaL_dostring( L, "sensors_map_i2c = {}" );
	luaL_dostring( L, "accelerometers = {}" );
	luaL_dostring( L, "gyroscopes = {}" );
	luaL_dostring( L, "magnetometers = {}" );
	luaL_dostring( L, "altimeters = {}" );
	luaL_dostring( L, "GPSes = {}" );
	luaL_dostring( L, "user_sensors = {}" );
	luaL_dostring( L, "function RegisterSensor( name, params ) user_sensors[name] = params ; return params end" );

	luaL_loadfile( L, mFilename.c_str() );
	int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );
	if ( ret != 0 ) {
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		gDebug() << "Lua : Error while executing file \"" << mFilename << "\" : " << ar.currentline << " : \"" << lua_tostring( L, -1 ) << "\"\n";
		return;
	}

	if ( mSettingsFilename != "" ) {
		luaL_loadfile( L, mSettingsFilename.c_str() );
		int ret = lua_pcall( L, 0, LUA_MULTRET, 0 );
		if ( ret != 0 ) {
			lua_Debug ar;
			lua_getstack(L, 1, &ar);
			lua_getinfo(L, "nSl", &ar);
			gDebug() << "Lua : Error while executing file \"" << mSettingsFilename << "\" : " << ar.currentline << " : \"" << lua_tostring( L, -1 ) << "\"\n";
		}
	
		std::string line;
		std::string key;
		std::string value;
		std::ifstream file( mSettingsFilename, std::ios_base::in );
		if ( file.is_open() ) {
			while ( std::getline( file, line ) ) {
				key = line.substr( 0, line.find( "=" ) );
				value = line.substr( line.find( "=" ) + 1 );
				key = trim( key );
				value = trim( value );
				mSettings[ key ] = value;
				std::cout << "mSettings[\"" << key << "\"] = '" << mSettings[ key ] << "'\n";
			}
		}
	}

	std::list< std::string > user_sensors;
	LocateValue( "user_sensors" );
	lua_pushnil( L );
	while( lua_next( L, -2 ) != 0 ) {
		std::string key = lua_tostring( L, -2 );
		user_sensors.emplace_back( key );
		lua_pop( L, 1 );
	}
	for ( std::string s : user_sensors ) {
		Sensor::RegisterDevice( s, this, "user_sensors." + s );
	}
}


void Config::Apply()
{
	std::list< std::string > accelerometers;
	std::list< std::string > gyroscopes;
	std::list< std::string > magnetometers;

	lua_getglobal( L, "accelerometers" );
	lua_pushnil( L );
	while ( lua_next( L, -2 ) ) {
		accelerometers.emplace_back( lua_tolstring( L, -2, nullptr ) );
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	lua_getglobal( L, "gyroscopes" );
	lua_pushnil( L );
	while ( lua_next( L, -2 ) ) {
		gyroscopes.emplace_back( lua_tolstring( L, -2, nullptr ) );
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	lua_getglobal( L, "magnetometers" );
	lua_pushnil( L );
	while ( lua_next( L, -2 ) ) {
		magnetometers.emplace_back( lua_tolstring( L, -2, nullptr ) );
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	for ( auto it : gyroscopes ) {
		Gyroscope* gyro = Sensor::gyroscope( it );
		if ( gyro ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = integer( "gyroscopes." + it + ".axis_swap.x" );
			swap[1] = integer( "gyroscopes." + it + ".axis_swap.y" );
			swap[2] = integer( "gyroscopes." + it + ".axis_swap.z" );
			gyro->setAxisSwap( swap );
		}
	}
	for ( auto it : accelerometers ) {
		Accelerometer* accel = Sensor::accelerometer( it );
		if ( accel ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = integer( "accelerometers." + it + ".axis_swap.x" );
			swap[1] = integer( "accelerometers." + it + ".axis_swap.y" );
			swap[2] = integer( "accelerometers." + it + ".axis_swap.z" );
			accel->setAxisSwap( swap );
		}
	}
	for ( auto it : magnetometers ) {
		Magnetometer* magn = Sensor::magnetometer( it );
		if ( magn ) {
			int swap[4] = { 0, 0, 0, 0 };
			swap[0] = integer( "magnetometers." + it + ".axis_swap.x" );
			swap[1] = integer( "magnetometers." + it + ".axis_swap.y" );
			swap[2] = integer( "magnetometers." + it + ".axis_swap.z" );
			magn->setAxisSwap( swap );
		}
	}
}


void Config::setBoolean( const std::string& name, const bool v )
{
	mSettings[name] = ( v ? "true" : "false" );
	luaL_dostring( L, ( name + "=" + mSettings[name] ).c_str() );
}


void Config::setInteger( const std::string& name, const int v )
{
	mSettings[name] = std::to_string(v);
	luaL_dostring( L, ( name + "=" + mSettings[name] ).c_str() );
}


void Config::setNumber( const std::string& name, const float v )
{
	mSettings[name] = std::to_string(v);
	luaL_dostring( L, ( name + "=" + mSettings[name] ).c_str() );
}


void Config::setString( const std::string& name, const std::string& v )
{
	mSettings[name] = v;
	luaL_dostring( L, ( name + "=" + v ).c_str() );
}


void Config::Save()
{
	std::ofstream file( mSettingsFilename );
	std::string str;

	if ( file.is_open() ) {
		for ( std::pair< std::string, std::string > setting : mSettings ) {
			str = setting.first + " = " + setting.second + "\n";
			gDebug() << "Saving setting  : " << str;
			file.write( str.c_str(), str.length() );
		}
		file.close();
	}
}
