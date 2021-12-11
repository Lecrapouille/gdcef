//*************************************************************************
// Stigmee: A 3D browser and decentralized social network.
// Copyright 2021 Alain Duron <duron.alain@gmail.com>
//
// This file is part of Stigmee.
//
// Stigmee is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//*************************************************************************

#include "apphandler.h"

#include <iostream>
#include <cef_client.h>
#include <cef_app.h>
#include <cef_helpers.h>

using namespace godot;


// NOTE: This class is mostly useless and serving testing purpose


//Returns module handle where this function is running in: EXE or DLL
HMODULE getStaticModuleHandle() {
	HMODULE hModule = NULL;
	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)getStaticModuleHandle, &hModule);
		return hModule;
}

AppHandler::AppHandler() {

	std::cout << "[AppHandler::AppHandler()]" << std::endl;
	std::cout << "Setting up new AppHandler and handing it to godot" << std::endl;
	//m_app = new AppHandler();

}

AppHandler::~AppHandler() {
	// add your cleanup here

}

void AppHandler::_init() {
	// initialize any variables here
}

void AppHandler::_register_methods() {
	// Not exposing anything yet
}
