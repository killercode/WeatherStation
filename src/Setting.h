// Object Representation of the setting key value pair
// This will be used to store settings on EEPROM

#ifndef SRC_SETTING_H_
#define SRC_SETTING_H_

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

#endif  // SRC_SETTING_H_
