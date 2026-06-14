#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdint>

/*
print rom 
xxd roms/tetris.gb | head -n 25
*/

// Forces the compiler to pack the struct tightly with NO padding bytes
#pragma pack(push, 1)
struct GBHeader 
{
    uint8_t  entry_point[4];       // $0100-$0103: Usually NOP + JP
    uint8_t  nintendo_logo[48];    // $0104-$0133: Logo bitmap
    char     title[11];            // $0134-$013E: Title string
    char     manufacturer_code[4]; // $013F-$0142: Newer manufacture code
    uint8_t  cgb_flag;             // $0143: Game Boy Color flag
    uint16_t new_licensee_code;    // $0144-$0145: Modern publisher code
    uint8_t  sgb_flag;             // $0146: Super Game Boy flag
    uint8_t  cartridge_type;       // $0147: MBC chip descriptor
    uint8_t  rom_size;             // $0148: ROM bank size mapping
    uint8_t  ram_size;             // $0149: RAM bank size mapping
    uint8_t  destination_code;     // $014A: Region target
    uint8_t  old_licensee_code;    // $014B: Legacy publisher code
    uint8_t  mask_rom_version;     // $014C: ROM revision number
    uint8_t  header_checksum;      // $014D: Critical verification byte
    uint16_t global_checksum;      // $014E-$014F: Complete file checksum
};
#pragma pack(pop)

void print_header(GBHeader& header)
{
    std::printf("Entry Point:     0x%02X %02X %02X %02X\n", 
                header.entry_point[0], header.entry_point[1], 
                header.entry_point[2], header.entry_point[3]);

    std::printf("Nintendo Logo: ");
    for (int i = 0; i < 48; i++)
    {
        if (i > 0 && i % 16 == 0)
            std::printf("\n               "); 
        std::printf("%02X ", header.nintendo_logo[i]);
    }
    std::printf("\n");
    
    std::printf("Title:           %.16s\n", header.title);
    std::printf("CGB Flag:        0x%02X\n", header.cgb_flag);
    std::printf("New Licensee:    0x%04X\n", header.new_licensee_code);
    std::printf("SGB Flag:        0x%02X\n", header.sgb_flag);
    std::printf("Cartridge Type:  0x%02X\n", header.cartridge_type);
    std::printf("ROM Size:        0x%02X\n", header.rom_size);
    std::printf("RAM Size:        0x%02X\n", header.ram_size);
    std::printf("Destination:     0x%02X\n", header.destination_code);
    std::printf("Old Licensee:    0x%02X\n", header.old_licensee_code);
    std::printf("Mask ROM Version:0x%02X\n", header.mask_rom_version);
    std::printf("Header Checksum: 0x%02X\n", header.header_checksum);
    std::printf("Global Checksum: 0x%04X\n", header.global_checksum);

    uint8_t calculated_checksum = 0;
    
    uint8_t* header_bytes = reinterpret_cast<uint8_t*>(&header);
    
    for (int i = 52; i <= 76; i++) 
    {
        calculated_checksum = calculated_checksum - header_bytes[i] - 1;
    }

    std::printf("Calculated Checksum: 0x%02X\n", calculated_checksum);
    
    if (calculated_checksum == header.header_checksum) 
    {
        std::printf("STATUS: Checksum PASS! Pass the execution over to the CPU.\n");
    } 
    else 
    {
        std::printf("STATUS: Checksum FAIL! System Lock up.\n");
    }
}

int main() 
{
    std::ifstream file("roms/tetris.gb", std::ios::binary);

    if (!file.is_open()) return 1;

    file.seekg(0x0100, std::ios::beg);

    GBHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    print_header(header);

    file.close();
    return 0;
}