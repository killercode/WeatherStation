// Object Representation of the setting key value pair
// This will be used to store settings on EEPROM

#ifndef Setting_h
#define Setting_h

class Setting 
{
	public:
		Setting(String key, String value);
		Setting();
		String getKey();
		void setKey(String k);
		String getValue();
		void setValue(String v);

	private:
		String key;
		String value;
};

#endif