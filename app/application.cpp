#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>

HttpServer server;
//FTPServer ftp;

BssList networks;
String network, password, currentNetwork;
Timer connectionTimer;


/* ---------------- */
#define	COM_PIN			13	// ICSPCLK

#define COM_READ_EEPROM		0x20
#define COM_WRITE_EEPROM	0xE0
#define COM_READ_TEMP		0x01
#define COM_READ_COOLING	0x02
#define COM_READ_HEATING	0x03
#define COM_ACK			0x9A
#define COM_NACK		0x66

void com_write_bit(unsigned const char data){
	pinMode(COM_PIN, OUTPUT);
	digitalWrite(COM_PIN, HIGH);
	delayMicroseconds(7);
	if(!data){
		pinMode(COM_PIN, INPUT);
		digitalWrite(COM_PIN, LOW);
	}
	delayMicroseconds(400);
	pinMode(COM_PIN, INPUT);
	digitalWrite(COM_PIN, LOW);
	delayMicroseconds(100);
}

unsigned char com_read_bit(){
	unsigned char data;

	pinMode(COM_PIN, OUTPUT);
	digitalWrite(COM_PIN, HIGH);
	delayMicroseconds(7);
	pinMode(COM_PIN, INPUT);
	digitalWrite(COM_PIN, LOW);
	delayMicroseconds(200);
	data = digitalRead(COM_PIN);
	delayMicroseconds(300);

	return data;
}

void com_write_byte(unsigned const char data){
	unsigned char i;
	
	for(i=0;i<8;i++){
		com_write_bit(((data << i) & 0x80));
	}
	delayMicroseconds(500);
}

unsigned char com_read_byte(){
	unsigned char i, data;
	
	for(i=0;i<8;i++){
		data <<= 1;
		if(com_read_bit()){
			data |= 1;
		}
	}
	delayMicroseconds(500);

	return data;
}


bool _com_write_eeprom(const unsigned char address, unsigned const int value){
	unsigned char ack;
	com_write_byte(COM_WRITE_EEPROM);
	com_write_byte(address);
	com_write_byte(((unsigned char)(value >> 8)));
	com_write_byte((unsigned char)value);
	com_write_byte(COM_WRITE_EEPROM ^ address ^ ((unsigned char)(value >> 8)) ^ ((unsigned char)value));
	delay(7); // Longer delay needed here for EEPROM write to finish, but must be shorter than 10ms
	ack = com_read_byte();
	return ack == COM_ACK; 
}

bool _com_read_eeprom(const unsigned char address, int *value){
	unsigned char xorsum;
	unsigned char ack;
        unsigned int data;

	com_write_byte(COM_READ_EEPROM);
	com_write_byte(address);
	data = com_read_byte();
	data = (data << 8) | com_read_byte();
	xorsum = com_read_byte();
        ack = com_read_byte();
	if(ack == COM_ACK && xorsum == (COM_READ_EEPROM ^ address ^ ((unsigned char)(data >> 8)) ^ ((unsigned char)data))){
	        *value = (int)data;
		return true;
	}
	return false;
}

bool com_write_eeprom(const unsigned char address, unsigned const int value){
	bool rv = false;
	unsigned int i=0;

	while(i<5 && !rv){
		rv = _com_write_eeprom(address, value);
		delay(16);
		if(!rv){
			int v;
			_com_read_eeprom(address, &v);
			rv = (v == value);
			delay(16);
		}
		i++;
	}
	return rv;
}

bool com_read_eeprom(const unsigned char address, int *value){
	bool rv = false;
	unsigned int i=0;

	while(i<5 && !rv){
		rv = _com_read_eeprom(address, value);
		delay(16);
		i++;
	}
	return rv;
}

bool com_read_command(unsigned char command, int *value){
	unsigned char xorsum;
	unsigned char ack;
        unsigned int data;

	com_write_byte(command);
	data = com_read_byte();
	data = (data << 8) | com_read_byte();
	xorsum = com_read_byte();
        ack = com_read_byte();
	if(ack == COM_ACK && xorsum == (command ^ ((unsigned char)(data >> 8)) ^ ((unsigned char)data))){
	        *value = (int)data;
		return true;
	}
	return false;
}

bool com_read_temp(int *temperature){
	return com_read_command(COM_READ_TEMP, temperature); 
}

bool com_read_heating(int *heating){
	return com_read_command(COM_READ_HEATING, heating); 
}

bool com_read_cooling(int *cooling){
	return com_read_command(COM_READ_COOLING, cooling); 
}

/* ---------------- */





/* ---- */

/* Define STC-1000+ version number (XYY, X=major, YY=minor) and EEROM revision */
#define STC1000P_MAGIC_F		0x192C
#define STC1000P_MAGIC_C		0x26D3
#define STC1000P_VERSION		108
#define STC1000P_EEPROM_VERSION		11

/* Pin configuration */
#define ICSPCLK 13
#define ICSPDAT 12
#define nMCLR   14 

/* Delays */
#define TDLY()  delayMicroseconds(1)    /* 1.0us minimum */
#define TCKH()  delayMicroseconds(1)    /* 100ns minimum */
#define TCKL()  delayMicroseconds(1)    /* 100ns minimum */
#define TERAB() delay(5)    		    /* 5ms maximum */
#define TERAR() delay(3)    		    /* 2.5ms maximum */
#define TPINT() delay(5)    		    /* 5ms maximum */
#define TPEXT() delay(2)                /* 1.0ms minimum, 2.1ms maximum*/
#define TDIS()  delayMicroseconds(1)    /* 300ns minimum */
#define TENTS() delayMicroseconds(1)    /* 100ns minimum */
#define TENTH() delay(4)  		        /* 250us minimum */ /* Needs a lot more when powered by arduino */
#define TEXIT() delayMicroseconds(1)    /* 1us minimum */

/* Commands */
#define LOAD_CONFIGURATION                  0x00    /* 0, data(14), 0 */
#define LOAD_DATA_FOR_PROGRAM_MEMORY        0x02    /* 0, data(14), 0 */
#define LOAD_DATA_FOR_DATA_MEMORY           0x03    /* 0, data(14), zero(6), 0 */
#define READ_DATA_FROM_PROGRAM_MEMORY       0x04    /* 0, data(14), 0 */
#define READ_DATA_FROM_DATA_MEMORY          0x05    /* 0, data(14), zero(6), 0 */
#define INCREMENT_ADDRESS                   0x06    /* - */
#define RESET_ADDRESS                       0x16    /* - */
#define BEGIN_INTERNALLY_TIMED_PROGRAMMING  0x08    /* - */
#define BEGIN_EXTERNALLY_TIMED_PROGRAMMING  0x18    /* - */
#define END_EXTERNALLY_TIMED_PROGRAMMING    0x0A    /* - */
#define BULK_ERASE_PROGRAM_MEMORY           0x09    /* Internally Timed */
#define BULK_ERASE_DATA_MEMORY              0x0B    /* Internally Timed */
#define ROW_ERASE_PROGRAM_MEMORY            0x11    /* Internally Timed */

file_t hexfile;

void prog_setup() {
	pinMode(ICSPCLK, INPUT);
	digitalWrite(ICSPCLK, LOW); // Disable pull-up
	pinMode(ICSPDAT, INPUT);
	digitalWrite(ICSPDAT, LOW); // Disable pull-up
	pinMode(nMCLR, INPUT);
	digitalWrite(nMCLR, LOW); // Disable pull-up
}

/* low level bit transfer */
void write_bit(unsigned char bit) {
	digitalWrite(ICSPCLK, HIGH);
	digitalWrite(ICSPDAT, bit ? HIGH : LOW);
	TCKH();
	digitalWrite(ICSPCLK, LOW);
	TCKL();
	digitalWrite(ICSPDAT,LOW); // REM?
}

unsigned char read_bit() {
	unsigned char rv;

	digitalWrite(ICSPCLK, HIGH);
	TCKH();
    rv = digitalRead(ICSPDAT);
	digitalWrite(ICSPCLK, LOW);
	TCKL();

	return rv;
}

void lvp_entry() {
	unsigned long LVP_magic = 0b01001101010000110100100001010000;
	unsigned char i;

	Serial.println("Enter low voltage programming mode");

	pinMode(nMCLR, OUTPUT);
	digitalWrite(nMCLR, HIGH);
	TENTS();
	digitalWrite(nMCLR, LOW); // REM
	pinMode(ICSPCLK, OUTPUT); // REM
	digitalWrite(ICSPCLK, LOW); // REM
	pinMode(ICSPDAT, OUTPUT); // REM
	digitalWrite(ICSPDAT, LOW);
	TENTH();

	// Send "MCHP" backwards, to unlock LVP mode
	for (i = 0; i < 32; i++) {
		write_bit((LVP_magic >> i) & 1);
	}
	write_bit(0);
}

void p_exit() {

	Serial.println("Leaving programming mode");

	digitalWrite(nMCLR, LOW); // LVP mode
	digitalWrite(ICSPCLK, LOW);
	digitalWrite(ICSPDAT, LOW);
	TEXIT();

	digitalWrite(nMCLR, HIGH); // LVP mode
	delay(1);
	pinMode(nMCLR, INPUT); // LVP mode

	pinMode(ICSPCLK, INPUT);
	pinMode(ICSPDAT, INPUT);
}

/* low level command transfer */
void write_command(unsigned char command) {
	unsigned char i;

	for (i = 0; i < 5; i++) {
		write_bit((command >> i) & 1);
	}
	write_bit(0);
	TDLY();
}

void write_command_with_data(unsigned char command, unsigned int data) {
	unsigned char i;

	write_command(command);

	write_bit(0);
	for (i = 0; i < 14; i++) {
		write_bit((data >> i) & 1);
	}
	write_bit(0);
}

unsigned int read_command(unsigned char command) {
	unsigned char i;
	unsigned int data = 0;

	write_command(command);

	pinMode(ICSPDAT, INPUT);

	read_bit();
	for (i = 0; i < 14; i++) {
		data |= ((unsigned int) (read_bit() & 0x1) << i);
	}
	read_bit();

	pinMode(ICSPDAT, OUTPUT);

	return data;
}

/* high level commands */
void load_configuration(unsigned int data) {
	write_command_with_data(LOAD_CONFIGURATION, data);
}

void load_data_for_program_memory(unsigned int data) {
	write_command_with_data(LOAD_DATA_FOR_PROGRAM_MEMORY, data);
}

void load_data_for_data_memory(unsigned char data) {
	write_command_with_data(LOAD_DATA_FOR_DATA_MEMORY, data);
}

unsigned int read_data_from_program_memory() {
	return read_command(READ_DATA_FROM_PROGRAM_MEMORY);
}

unsigned char read_data_from_data_memory() {
	return (unsigned char) read_command(READ_DATA_FROM_DATA_MEMORY);
}

void increment_address() {
	write_command(INCREMENT_ADDRESS);
}

void reset_address() {
	write_command(RESET_ADDRESS);
}

void begin_internally_timed_programming() {
	write_command(BEGIN_INTERNALLY_TIMED_PROGRAMMING);
	TPINT();
}

void begin_externally_timed_programming() {
	write_command(BEGIN_EXTERNALLY_TIMED_PROGRAMMING);
	TPEXT();
}

void end_externally_timed_programming() {
	write_command(END_EXTERNALLY_TIMED_PROGRAMMING);
	TDIS();
}

void bulk_erase_program_memory() {
	Serial.println("Bulk erasing program memory");
	write_command(BULK_ERASE_PROGRAM_MEMORY);
	TERAB();
}

void bulk_erase_data_memory() {
	Serial.println("Bulk erasing data memory");
	write_command(BULK_ERASE_DATA_MEMORY);
	TERAB();
}

void row_erase_program_memory() {
	write_command(ROW_ERASE_PROGRAM_MEMORY);
	TERAR();
}

/* algorithms */
void bulk_erase_device() {
	Serial.println("Bulk erasing device");
	load_configuration(0);
	bulk_erase_program_memory();
	bulk_erase_data_memory();
}

void get_device_id(unsigned int *magic, unsigned int *version,
		unsigned int *deviceid) {
	lvp_entry();
	load_configuration(0);
	*magic = read_data_from_program_memory();
	increment_address();
	*version = read_data_from_program_memory();
	increment_address();

	increment_address();
	increment_address();
	increment_address();
	increment_address();

	*deviceid = read_data_from_program_memory();

	p_exit();
}

void write_magic(unsigned int data_word_out) {
	Serial.println("Writing magic.");
	load_configuration(0);
	load_data_for_program_memory(data_word_out);
	begin_internally_timed_programming();
}

void write_version(unsigned int data_word_out) {
	Serial.println("Writing version.");
	load_configuration(0);
	increment_address();
	load_data_for_program_memory(data_word_out);
	begin_internally_timed_programming();
}

unsigned char parse_hex_nibble(unsigned char data) {
	data = toupper(data);
	return (data >= 'A' ? data - 'A' + 10 : data - '0') & 0xf;
}

unsigned char parse_hex(unsigned char u, unsigned char l) {
	unsigned char data;
	data = parse_hex_nibble(u) << 4;
	data |= parse_hex_nibble(l);

	return data;
}

unsigned char handle_hex_data(unsigned char bytecount,
		unsigned int address, unsigned char recordtype, unsigned char data[]) {
	static unsigned int device_address = 0;
	unsigned char i;

	if (recordtype == 1) {
		reset_address();
		device_address = 0;
		Serial.println("Programming done");
		return 1;
	} else if (recordtype == 04) {
		if (data[1] == 0) {
			Serial.println("Programming program memory");
			reset_address();
			device_address = 0;
		} else if (data[1] == 1) {
			Serial.println("Programming config memory");
			load_configuration(0);
			device_address = 0;
		}
	} else if (recordtype == 00) {
		if (address >= 0xE000) {
			Serial.print("Programming ");
			Serial.print(bytecount >> 1, DEC);
			Serial.print(" bytes at EEPROM address 0x");
			Serial.println((address & 0x1FFF) >> 1, HEX);

			if (device_address != ((address & 0x1FFF) >> 1)
					|| ((address & 0x1FFF) >> 1) == 0) {
				Serial.println("Resetting address for EEPROM");
				reset_address();
				device_address = 0;
			}
			while (device_address != ((address & 0x1FFF) >> 1)) {
				increment_address();
				device_address++;
				Serial.print("Incrementing address to 0x");
				Serial.println(device_address, HEX);
			}

			for (i = 0; i < bytecount; i += 2) {
				unsigned char data_out = data[i];
				unsigned char data_in;
				load_data_for_data_memory(data_out);
				begin_internally_timed_programming();
				data_in = read_data_from_data_memory();
				if (data_in != data_out) {
					Serial.print("Validation failed for EEPROM address 0x");
					Serial.print(device_address, HEX);
					Serial.print(" wrote 0x");
					Serial.print(data_out, HEX);
					Serial.print(" but read back 0x");
					Serial.println(data_in, HEX);
					return 1;
				}
				increment_address();
				device_address++;
			}
		} else {
			Serial.print("Programming ");
			Serial.print(bytecount >> 1, DEC);
			Serial.print(" words at address 0x");
			Serial.println(address >> 1, HEX);

			while (device_address != (address >> 1)) {
				increment_address();
				device_address++;
				Serial.print("Incrementing address to 0x");
				Serial.println(device_address, HEX);
			}

			for (i = 0; i < bytecount; i += 2) {
				unsigned int data_word_out = (((unsigned int) data[i + 1]) << 8)
						| data[i];
				unsigned int data_word_in;
				load_data_for_program_memory(data_word_out);
				begin_internally_timed_programming();
				data_word_in = read_data_from_program_memory();
				if (data_word_in != data_word_out) {
					Serial.print("Validation failed for address 0x");
					Serial.print(device_address, HEX);
					Serial.print(" wrote 0x");
					Serial.print(data_word_out, HEX);
					Serial.print(" but read back 0x");
					Serial.println(data_word_in, HEX);
					return 1;
				}
				increment_address();
				device_address++;
			}
		}
	}
	return 0;
}

unsigned char handle_hex_file_line(const char *buf){

	unsigned char bytecount;
	unsigned int address;
	unsigned char recordtype;
	unsigned char data[16];
	unsigned char checksum;
	unsigned char i, j;

	// Read start of line
	for(i=0; i<64; i++){
		if(buf[i] == '\0'){
			return 1;
		}
		if (buf[i] == ':') {
			break;
		}
	}

	if(i>=52){
		return 1;
	}

	i++;

	// Read bytecount
	bytecount = parse_hex(buf[i++], buf[i++]);
	checksum = bytecount;

	// Read address
	j = parse_hex(buf[i++], buf[i++]);
	address = ((unsigned int) j) << 8;
	checksum += j;
	j = parse_hex(buf[i++], buf[i++]);
	address |= j;
	checksum += j;

	// Read recordtype
	recordtype = parse_hex(buf[i++], buf[i++]);
	checksum += recordtype;

	for (j = 0; j < bytecount; j++) {
		data[j] = parse_hex(buf[i++], buf[i++]);
		checksum += data[j];
	}

	// Read checksum
	j = parse_hex(buf[i++], buf[i++]);
	checksum += j;

	if (checksum) {
		Serial.println("Checksum error!");
		return 1;
	}
	WDT.alive(); // Feed the dog

	return handle_hex_data(bytecount, address, recordtype, data);
}

/*
void upload_hex_file_to_device(String filename) {
	unsigned char done = 0;
	char line[64];

	hexfile=fileOpen(filename.c_str(), eFO_ReadOnly);

	Serial.println("Reading hex data...");

	// Read start of line
	while (!done) {
		unsigned char c;
		done = fileRead(hexfile, &c, 1) == 0;
		if (c == ':') {
			unsigned char i=0;
			line[0]=c;
			for(i=1; i<64; i++){
				fileRead(hexfile, &c, 1);
				if(c=='\0' || c=='\n' || c=='\r')
					break;
				line[i] = c;
			}
			line[i] = '\0';
			done = handle_hex_file_line(line);
		}
	}

	fileClose(hexfile);
}
*/


/*---- */




void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars = tmpl->variables();
	response.sendTemplate(tmpl); // will be automatically deleted
}

/* Deprecated */
/*
void onUpload(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("upload.html");
	auto &vars = tmpl->variables();

    {
			unsigned int magic, ver, deviceid;
			get_device_id(&magic, &ver, &deviceid);
			Serial.print("Device ID is: 0x");
			Serial.println(deviceid, HEX);
			if ((deviceid & 0x3FE0) == 0x27C0) {
				Serial.println("STC-1000 detected.");
				if (magic == STC1000P_MAGIC_C || magic == STC1000P_MAGIC_F) {
					Serial.print("STC-1000+ ");
					if (magic == STC1000P_MAGIC_F) {
						Serial.print("Fahrenheit ");
					} else {
						Serial.print("Celsius ");
					}
					Serial.print("firmware with version ");
					Serial.print(ver / 100, DEC);
					Serial.print(".");
					Serial.print((ver % 100) / 10, DEC);
					Serial.print((ver % 10), DEC);
					Serial.println(" detected.");

				} else {
					Serial.println("No previous STC-1000+ firmware detected.");
					Serial.println(
							"Consider initializing EEPROM when flashing.");
				}
				Serial.print("Sketch has version ");
				Serial.print(STC1000P_VERSION / 100, DEC);
				Serial.print(".");
				Serial.print((STC1000P_VERSION % 100) / 10, DEC);
				Serial.print((STC1000P_VERSION % 10), DEC);
				Serial.println("");
				Serial.println("");

               	lvp_entry();
               	load_configuration(0);
               	bulk_erase_program_memory();
               	reset_address();
               	upload_hex_file_to_device("stc1000p.hex");
               	write_magic(STC1000P_MAGIC_C);
               	write_version(STC1000P_VERSION);
               	p_exit();

			} else {
				Serial.println("STC-1000 NOT detected. Check wiring.");
			}
				
	}

	response.sendTemplate(tmpl); // will be automatically deleted
}
*/

void onProfile(HttpRequest &request, HttpResponse &response)
{
	unsigned int prno = 0;
	unsigned int i, addr;
	int v;
	float f;

	if (request.getRequestMethod() == RequestMethod::POST)
	{
		prno = atoi(request.getPostParameter("Pr", "0").c_str());

		addr = 19*prno;

		for(i=0; i<9; i++){
			f = atof(request.getPostParameter(String("sp") + String(i), "0").c_str());
			com_write_eeprom(addr++, (int)(f*10.0));
			v = atoi(request.getPostParameter(String("dh") + String(i), "0").c_str());
			com_write_eeprom(addr++, v);
		}
		f = atof(request.getPostParameter("sp9", "0").c_str());
		com_write_eeprom(addr++, (int)(f*10.0));

	} else if (request.getRequestMethod() == RequestMethod::GET) {
		prno = atoi(request.getQueryParameter("Pr", "0").c_str());
	}

	TemplateFileStream *tmpl = new TemplateFileStream("profile.html");
	auto &vars = tmpl->variables();

	vars["Pr"] = String(prno);
	vars["scale"] = String("C"); // TODO: AppSetting?

	addr = 19*prno;
	for(i=0; i<9; i++){
		com_read_eeprom(addr++, &v);
		vars[String("sp") + String(i)] = String(v/10) + "." + String(v%10);
		com_read_eeprom(addr++, &v);
		vars[String("dh") + String(i)] = String(v);
	}
	com_read_eeprom(addr++, &v);
	vars["sp9"] = String(v/10) + "." + String(v%10);

	response.sendTemplate(tmpl); // will be automatically deleted

}

void onSet(HttpRequest &request, HttpResponse &response)
{
	unsigned int prno = 0;
	unsigned int i, addr;
	int v;
	float f;

	if (request.getRequestMethod() == RequestMethod::POST)
	{
		prno = atoi(request.getPostParameter("Pr", "0").c_str());
		addr = 19*6;

		f = atof(request.getPostParameter(String("SP"), "0").c_str());
		com_write_eeprom(addr++, (int)(f*10.0));
		f = atof(request.getPostParameter(String("hy"), "0").c_str());
		com_write_eeprom(addr++, (int)(f*10.0));
		f = atof(request.getPostParameter(String("tc"), "0").c_str());
		com_write_eeprom(addr++, (int)(f*10.0));
		f = atof(request.getPostParameter(String("SA"), "0").c_str());
		com_write_eeprom(addr++, (int)(f*10.0));
		v = atoi(request.getPostParameter(String("St"), "0").c_str());
		com_write_eeprom(addr++, v);
		v = atoi(request.getPostParameter(String("dh"), "0").c_str());
		com_write_eeprom(addr++, v);
		v = atoi(request.getPostParameter(String("cd"), "0").c_str());
		com_write_eeprom(addr++, v);
		v = atoi(request.getPostParameter(String("hd"), "0").c_str());
		com_write_eeprom(addr++, v);
		v = atoi(request.getPostParameter(String("rP"), "0").c_str());
		com_write_eeprom(addr++, v);
		v = atoi(request.getPostParameter(String("rn"), "0").c_str());
		com_write_eeprom(addr++, v);

	}

	TemplateFileStream *tmpl = new TemplateFileStream("set.html");
	auto &vars = tmpl->variables();

	addr = 19*6;

	com_read_eeprom(addr++, &v);
	vars["SP"] = String(v/10) + "." + String(v%10);
	com_read_eeprom(addr++, &v);
	vars["hy"] = String(v/10) + "." + String(v%10);
	com_read_eeprom(addr++, &v);
	vars["tc"] = String(v/10) + "." + String(v%10);
	com_read_eeprom(addr++, &v);
	vars["SA"] = String(v/10) + "." + String(v%10);

	com_read_eeprom(addr++, &v);
	vars["St"] = String(v);
	com_read_eeprom(addr++, &v);
	vars["dh"] = String(v);
	com_read_eeprom(addr++, &v);
	vars["cd"] = String(v);
	com_read_eeprom(addr++, &v);
	vars["hd"] = String(v);
	com_read_eeprom(addr++, &v);
	vars["rP"] = String(v);
	com_read_eeprom(addr++, &v);
	vars["rn"] = String(v);

	response.sendTemplate(tmpl); // will be automatically deleted

}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.dhcp = request.getPostParameter("dhcp") == "1";
		AppSettings.ip = request.getPostParameter("ip");
		AppSettings.netmask = request.getPostParameter("netmask");
		AppSettings.gateway = request.getPostParameter("gateway");
		debugf("Updating IP settings: %d", AppSettings.ip.isNull());
		AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
	auto &vars = tmpl->variables();

	bool dhcp = WifiStation.isEnabledDHCP();
	vars["dhcpon"] = dhcp ? "checked='checked'" : "";
	vars["dhcpoff"] = !dhcp ? "checked='checked'" : "";

	if (!WifiStation.getIP().isNull())
	{
		vars["ip"] = WifiStation.getIP().toString();
		vars["netmask"] = WifiStation.getNetworkMask().toString();
		vars["gateway"] = WifiStation.getNetworkGateway().toString();
	}
	else
	{
		vars["ip"] = "192.168.1.77";
		vars["netmask"] = "255.255.255.0";
		vars["gateway"] = "192.168.1.1";
	}

	response.sendTemplate(tmpl); // will be automatically deleted
}

void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void onAjaxNetworkList(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["status"] = (bool)true;

	bool connected = WifiStation.isConnected();
	json["connected"] = connected;
	if (connected)
	{
		json["network"] = WifiStation.getSSID();
	}

	JsonArray& netlist = json.createNestedArray("available");
	for (int i = 0; i < networks.count(); i++)
	{
		if (networks[i].hidden) continue;
		JsonObject &item = netlist.createNestedObject();
		item["id"] = (int)networks[i].getHashId();
		// Copy full string to JSON buffer memory
		item["title"] = networks[i].ssid;
		item["signal"] = networks[i].rssi;
		item["encryption"] = networks[i].getAuthorizationMethodName();
	}


	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void makeConnection()
{
	WifiStation.enable(true);
	WifiStation.config(network, password);

	AppSettings.ssid = network;
	AppSettings.password = password;
	AppSettings.save();

	network = ""; // completed
}

void onAjaxConnect(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	String curNet = request.getPostParameter("network");
	String curPass = request.getPostParameter("password");

	bool updating = curNet.length() > 0 && (WifiStation.getSSID() != curNet || WifiStation.getPassword() != curPass);
	bool connectingNow = WifiStation.getConnectionStatus() == eSCS_Connecting || network.length() > 0;

	if (updating && connectingNow)
	{
		debugf("wrong action: %s %s, (updating: %d, connectingNow: %d)", network.c_str(), password.c_str(), updating, connectingNow);
		json["status"] = (bool)false;
		json["connected"] = (bool)false;
	}
	else
	{
		json["status"] = (bool)true;
		if (updating)
		{
			network = curNet;
			password = curPass;
			debugf("CONNECT TO: %s %s", network.c_str(), password.c_str());
			json["connected"] = false;
			connectionTimer.initializeMs(1200, makeConnection).startOnce();
		}
		else
		{
			json["connected"] = WifiStation.isConnected();
			debugf("Network already selected. Current status: %s", WifiStation.getConnectionStatusName());
		}
	}

	if (!updating && !connectingNow && WifiStation.isConnectionFailed())
		json["error"] = WifiStation.getConnectionStatusName();

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void onAjaxReadTemp(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();
	int temperature;
	com_read_temp(&temperature);

	json["temperature"] = String(temperature);

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void onAjaxReadEeprom(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	String adr = request.getPostParameter("address");
	int iadr = atoi(adr.c_str());
	int val;

	com_read_eeprom(iadr, &val);

	json["value"] = String(val);

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void onAjaxWriteEeprom(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	String adr = request.getPostParameter("address");
	String val = request.getPostParameter("value");
	int iadr = atoi(adr.c_str());
	int ival = atoi(val.c_str());

	com_write_eeprom(iadr, ival);

	json["value"] = String(ival);

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}


/* Websocket */
void wsConnected(WebSocket& socket)
{
	Serial.println("Websocket connected");
}

void wsMessageReceived(WebSocket& socket, const String& message)
{
	static int line = 0;

	if(String("hexflash").equals(message)){
		unsigned int magic, ver, deviceid;
		line = 0;

		get_device_id(&magic, &ver, &deviceid);
		Serial.print("Device ID is: 0x");
		Serial.println(deviceid, HEX);
		if ((deviceid & 0x3FE0) == 0x27C0) {
			Serial.println("STC-1000 detected.");
			if (magic == STC1000P_MAGIC_C || magic == STC1000P_MAGIC_F) {
				Serial.print("STC-1000+ ");
				if (magic == STC1000P_MAGIC_F) {
					Serial.print("Fahrenheit ");
				} else {
					Serial.print("Celsius ");
				}
				Serial.print("firmware with version ");
				Serial.print(ver / 100, DEC);
				Serial.print(".");
				Serial.print((ver % 100) / 10, DEC);
				Serial.print((ver % 10), DEC);
				Serial.println(" detected.");
			} else {
				Serial.println("No previous STC-1000+ firmware detected.");
				Serial.println(
						"Consider initializing EEPROM when flashing.");
			}
			Serial.print("Sketch has version ");
			Serial.print(STC1000P_VERSION / 100, DEC);
			Serial.print(".");
			Serial.print((STC1000P_VERSION % 100) / 10, DEC);
			Serial.print((STC1000P_VERSION % 10), DEC);
			Serial.println("");
			Serial.println("");

			WDT.alive();

		       	lvp_entry();
		       	load_configuration(0);
		       	bulk_erase_program_memory();
		       	reset_address();

			Serial.println("Send " + line);
			socket.sendString(String(line++));
		} else {
			Serial.println("Send -1");
			socket.sendString("-1");
 		}
	} else if(line) {
		if(handle_hex_file_line(message.c_str())){
		       	write_magic(STC1000P_MAGIC_C);
		       	write_version(STC1000P_VERSION);
		       	p_exit();
			socket.sendString(String("done"));
		} else {
			socket.sendString(String(line++));
		}
	}
}

void wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size)
{
	Serial.printf("Websocket binary data recieved, size: %d\r\n", size);
}

void wsDisconnected(WebSocket& socket)
{
}




void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/ipconfig", onIpConfig);
	server.addPath("/profile", onProfile);
	server.addPath("/set", onSet);
	server.addPath("/ajax/get-networks", onAjaxNetworkList);
	server.addPath("/ajax/connect", onAjaxConnect);
	server.addPath("/ajax/readtemp", onAjaxReadTemp);
	server.addPath("/ajax/readeeprom", onAjaxReadEeprom);
	server.addPath("/ajax/writeeeprom", onAjaxWriteEeprom);
	server.setDefaultHandler(onFile);

// Web Sockets configuration
	server.enableWebSockets(true);
	server.setWebSocketConnectionHandler(wsConnected);
	server.setWebSocketMessageHandler(wsMessageReceived);
	server.setWebSocketBinaryHandler(wsBinaryReceived);
	server.setWebSocketDisconnectionHandler(wsDisconnected);

}

/*
void startFTP()
{
	if (!fileExist("index.html"))
		fileSetContent("index.html", "<h3>Please connect to FTP and upload files from folder 'web/build' (details in code)</h3>");

	// Start FTP server
	ftp.listen(21);
	ftp.addUser("me", "123"); // FTP account
}
*/

// Will be called when system initialization was completed
void startServers()
{
//	startFTP();
	startWebServer();
}

void networkScanCompleted(bool succeeded, BssList list)
{
	if (succeeded)
	{
		for (int i = 0; i < list.count(); i++)
			if (!list[i].hidden && list[i].ssid.length() > 0)
				networks.add(list[i]);
	}
	networks.sort([](const BssInfo& a, const BssInfo& b){ return b.rssi - a.rssi; } );
}

void init()
{
    prog_setup();

	spiffs_mount(); // Mount file system, in order to work with files

	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Enable debug output to serial
	AppSettings.load();

	WifiStation.enable(true);
	if (AppSettings.exist())
	{
		WifiStation.config(AppSettings.ssid, AppSettings.password);
		if (!AppSettings.dhcp && !AppSettings.ip.isNull())
			WifiStation.setIP(AppSettings.ip, AppSettings.netmask, AppSettings.gateway);
	}
	WifiStation.startScan(networkScanCompleted);

	// Start AP for configuration
	int startAP = 1; // = 0;
//	com_read_eeprom(127, &startAP);
	WifiAccessPoint.enable(startAP == 1);
	WifiAccessPoint.config("STC1000", "", AUTH_OPEN);

    // Run WEB server on system ready
	System.onReady(startServers);
}
