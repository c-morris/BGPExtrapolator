#include "AS.h"

AS::AS() {}

void AS::setASN(uint32_t myasn) {
    asn = myasn;
}

void AS::printDebug() {
    std::cout << asn << std::endl;
}
