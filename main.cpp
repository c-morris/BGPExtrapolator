#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>

#include <iostream>
#include <string.h>
#include <fstream>
#include <vector>

#define HMAC_KEY_LENGTH 32

int calculateECDSAByteLength(int path_length) {
    //4 bytes: target AS Number
    //Signature Segment: 20 (SKI) + 2(length) + 32(512 bit signature) -> for each hop (number of ASes on the path - 1)
    //Path Segment: 6 bytes -> Each AS on the path
    //Suite Identifier: 1 Byte
    //AFI: 4 Bytes (bottom of page 4)
    //SAFI: 2 Bytes
    //NLRI: 1 Byte for prefix length and 8 bytes for the prefix itself
    return 20 + (54 * (path_length - 1) + (6 * path_length));
}

int calculateHMACByteLength(int path_length) {
    //Prefix: 8
    //AS number: 4 bytes, HMAC signature: 32 bytes -> For each AS in the path
    return 8 + (path_length * 36);
}

void calculateECDSARuntime(double &ecdsa_verification_time_total, double &ecdsa_signature_time_total, int num_announcements, EC_KEY* key, unsigned char* messege, int messege_size, unsigned char* hash ) {
    ecdsa_verification_time_total = 0;
    ecdsa_signature_time_total = 0;

    clock_t ecdsaSignStart, ecdsaSignEnd;
    clock_t ecdsaVerifyStart, ecdsaVerifyEnd;

    //ECDSA p-256 with SHA-256
    for(int t = 0; t < num_announcements; t++) {
        //Alter messege
        RAND_bytes(messege, messege_size);

        //Re-hash
        SHA256(messege, messege_size, hash);

        //Get new key
        EC_KEY_generate_key(key);

        ecdsaSignStart = clock();

        //Sign it. Measure this as well
        ECDSA_SIG* signature = ECDSA_do_sign(hash, SHA256_DIGEST_LENGTH, key);

        ecdsaSignEnd = clock();

        ecdsaVerifyStart = clock();

        //Verify
        ECDSA_do_verify(hash, SHA256_DIGEST_LENGTH, signature, key);

        ecdsaVerifyEnd = clock();

        ecdsa_verification_time_total += (double) (ecdsaVerifyEnd - ecdsaVerifyStart) / CLOCKS_PER_SEC;
        ecdsa_signature_time_total += (double) (ecdsaSignEnd - ecdsaSignStart) / CLOCKS_PER_SEC;

        //cleanup
        ECDSA_SIG_free(signature);
    }
}

void calculateHMACRuntime(double &hmac_verification_time_total, double &hmac_signature_time_total, int num_announcements, unsigned char* hkey, unsigned char* messege, int messege_size, unsigned char* fake_original_auth_code) {
    hmac_verification_time_total = 0;
    hmac_signature_time_total = 0;
    
    clock_t hmacSignStart, hmacSignEnd;
    clock_t hmacVerifyStart, hmacVerifyEnd;

    for(int t = 0; t < num_announcements; t++) {
        //New Key
        RAND_bytes(hkey, HMAC_KEY_LENGTH);

        //Alter messege
        RAND_bytes(messege, messege_size);

        hmacSignStart = clock();

        unsigned char* auth_code = HMAC(EVP_sha256(), hkey, HMAC_KEY_LENGTH, messege, messege_size, NULL, NULL);

        hmacSignEnd = clock();

        //Since we need to compare the code to the "original", copy this into a new variable (off the timer) to then compare against on the timer
        memcpy(fake_original_auth_code, auth_code, SHA256_DIGEST_LENGTH);

        hmacVerifyStart = clock();

        if(memcmp(fake_original_auth_code, auth_code, SHA256_DIGEST_LENGTH) != 0) {
            printf("Unequal\n");
        }

        hmacVerifyEnd = clock();

        hmac_verification_time_total += (double) ((hmacSignEnd - hmacSignStart) + (hmacVerifyEnd - hmacVerifyStart)) / CLOCKS_PER_SEC;
        hmac_signature_time_total += (double) (hmacSignEnd - hmacSignStart) / CLOCKS_PER_SEC;
    }
}

//dynamic path length, check length in different versions of BGPsec!
//Also measure time to sign
//frequency of anouncements? - Cameron has a paper
int main(int argc, char *argv[]) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    std::vector<int> num_announcements{1000, 5000, 10000, 50000, 80000, 100000};

    std::ofstream ecdsaFile("ECDSA.csv");
    std::ofstream hmacFile("HMAC.csv");

    std::ofstream ecdsaLengthFile("ECDSALength.csv");
    std::ofstream hmacLengthFile("HMACLength.csv");

    //variables for running algorithms
    unsigned char hash[SHA256_DIGEST_LENGTH];

    unsigned char hkey[HMAC_KEY_LENGTH];
    unsigned char fake_original_auth_code[SHA256_DIGEST_LENGTH];

    //timing variables
    double ecdsa_verification_time_total = 0;
    double ecdsa_signature_time_total = 0;

    double hmac_verification_time_total = 0;
    double hmac_signature_time_total = 0;

    //Calculate time to crunch verifications and signatures at different number of announcements
    for(int path_length = 1; path_length <= 8; path_length++) {
        int ecdsaByteLength = calculateECDSAByteLength(path_length);
        int hmacByteLength = calculateHMACByteLength(path_length);

        ecdsaFile << "Path Length: " << path_length << " (" << ecdsaByteLength << " bytes),ECDSA Signature,ECDSA Verification\n";
        hmacFile << "Path Length: " << path_length << " (" << hmacByteLength << " bytes),HMAC Signature,HMAC Verification\n";

        unsigned char hmacMessege[hmacByteLength];
        unsigned char ecdsaMessege[ecdsaByteLength];

        for(unsigned int i = 0; i < num_announcements.size(); i++) {
            calculateECDSARuntime(ecdsa_verification_time_total, ecdsa_signature_time_total, num_announcements[i], key, ecdsaMessege, ecdsaByteLength, hash);
            ecdsaFile << num_announcements[i] << "," << ecdsa_signature_time_total << "," << ecdsa_verification_time_total << "\n";

            calculateHMACRuntime(hmac_verification_time_total, hmac_signature_time_total, num_announcements[i], hkey, hmacMessege, hmacByteLength, fake_original_auth_code);
            hmacFile << num_announcements[i] << "," << hmac_signature_time_total << "," << hmac_verification_time_total<< "\n";
        }

        ecdsaFile << "\n";
        hmacFile << "\n";
    }

    //Calculate time to crunch verifications and signatures at different byte lengths
    for(unsigned int i = 0; i < num_announcements.size(); i++) {
        ecdsaLengthFile << num_announcements[i] << " Announcements," << "ECDSA Signature,ECDSA Verification\n";
        hmacLengthFile << num_announcements[i] << " Announcements," << "HMAC Signature,HMAC Verification\n";

        for(int path_length = 1; path_length <= 8; path_length++) {
            int ecdsaByteLength = calculateECDSAByteLength(path_length);
            int hmacByteLength = calculateHMACByteLength(path_length);

            unsigned char hmacMessege[hmacByteLength];
            unsigned char ecdsaMessege[ecdsaByteLength];

            calculateECDSARuntime(ecdsa_verification_time_total, ecdsa_signature_time_total, num_announcements[i], key, ecdsaMessege, ecdsaByteLength, hash);
            ecdsaLengthFile << path_length << ", " << ecdsa_signature_time_total << "," << ecdsa_verification_time_total << "\n"; 

            calculateHMACRuntime(hmac_verification_time_total, hmac_signature_time_total, num_announcements[i], hkey, hmacMessege, hmacByteLength, fake_original_auth_code);
            hmacLengthFile << path_length << "," << hmac_signature_time_total << "," << hmac_verification_time_total<< "\n";
        }

        ecdsaLengthFile << "\n";
        hmacLengthFile << "\n";
    }

    //cleanup
    EC_KEY_free(key);

    return 0;
}
