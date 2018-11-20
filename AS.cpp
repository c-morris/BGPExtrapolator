#include "AS.h"

AS::AS() {}

AS::AS(uint32_t myasn) {
    asn = myasn;
}

void AS::setASN(uint32_t myasn) {
    asn = myasn;
}

void AS::printDebug() {
    std::cout << asn << std::endl;
}
