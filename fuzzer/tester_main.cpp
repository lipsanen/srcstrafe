#include <cstdio>
#include <stdio.h>
#include <stdint.h>
#include <cstdlib>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int main() 
{
    uint8_t BUFFER[4096];
    size_t size = fread(BUFFER, 1, sizeof(BUFFER), stdin);
    return LLVMFuzzerTestOneInput(BUFFER, size);
}
