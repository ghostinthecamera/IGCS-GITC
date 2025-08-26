#pragma once

namespace IGCS::IGCSConnector
{
	static std::vector<LPBYTE> _dataBlocksFromToolsToAddOns;
	static std::vector<HMODULE> _addonModules;

	void setupIGCSConnectorConnection();
	void connectWithReshadeAddons();
	void fillDataForAddOn();
}