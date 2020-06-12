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

void calculateECDSARuntime(double &ecdsa_verification_time_total, double &ecdsa_signature_time_total, int num_times, EC_KEY* key, int path_length, unsigned char* hash) {
    ecdsa_verification_time_total = 0;
    ecdsa_signature_time_total = 0;

    clock_t ecdsaSignStart, ecdsaSignEnd;
    clock_t ecdsaVerifyStart, ecdsaVerifyEnd;

    //ECDSA p-256 with SHA-256
    for(int t = 0; t < num_times; t++) {
        int signature_messege_length = 20 + (54 * (path_length - 1) + (6 * path_length));

        unsigned char message_sign[signature_messege_length];

        RAND_bytes(message_sign, signature_messege_length);
        SHA256(message_sign, signature_messege_length, hash);

        EC_KEY_generate_key(key);

        ecdsaSignStart = clock();

        ECDSA_SIG* sig = ECDSA_do_sign(hash, SHA256_DIGEST_LENGTH, key);

        ecdsaSignEnd = clock();

        ecdsa_signature_time_total += (double) (ecdsaSignEnd - ecdsaSignStart) / CLOCKS_PER_SEC;

        ECDSA_SIG_free(sig);

        for(int length = 1; length <= path_length - 1; length++) {
            int messege_length = 20 + (54 * (path_length - 1) + (6 * path_length));

            unsigned char message[signature_messege_length];

            RAND_bytes(message, messege_length);
            SHA256(message, messege_length, hash);

            EC_KEY_generate_key(key);

            ECDSA_SIG* signature = ECDSA_do_sign(hash, SHA256_DIGEST_LENGTH, key);

            ecdsaVerifyStart = clock();
            ECDSA_do_verify(hash, SHA256_DIGEST_LENGTH, signature, key);
            ecdsaVerifyEnd = clock();

            ecdsa_verification_time_total += (double) (ecdsaVerifyEnd - ecdsaVerifyStart) / CLOCKS_PER_SEC;

            //cleanup
            ECDSA_SIG_free(signature);
        }
    }
}

void calculateHMACRuntime(double &hmac_verification_time_total, double &hmac_signature_time_total, int num_times, unsigned char* hkey, int path_length, unsigned char* fake_original_auth_code) {
    hmac_verification_time_total = 0;
    hmac_signature_time_total = 0;
    
    clock_t hmacSignStart, hmacSignEnd;
    clock_t hmacVerifyStart, hmacVerifyEnd;

    for(int t = 0; t < num_times; t++) {
        int signature_messege_length = 8 + (path_length * 36);

        unsigned char message_sign[signature_messege_length];

        RAND_bytes(hkey, HMAC_KEY_LENGTH);
        RAND_bytes(message_sign, signature_messege_length);

        hmacSignStart = clock();
        unsigned char* auth_code = HMAC(EVP_sha256(), hkey, HMAC_KEY_LENGTH, message_sign, signature_messege_length, NULL, NULL);
        hmacSignEnd = clock();

        // free(auth_code);
        hmac_signature_time_total += (double) (hmacSignEnd - hmacSignStart) / CLOCKS_PER_SEC;

        for(int length = 1; length <= path_length - 1; length++) {
            int messege_length = 8 + (path_length * 36);
            unsigned char message[messege_length];

            RAND_bytes(hkey, HMAC_KEY_LENGTH);
            RAND_bytes(message, messege_length);

            hmacSignStart = clock();

            unsigned char* auth = HMAC(EVP_sha256(), hkey, HMAC_KEY_LENGTH, message, messege_length, NULL, NULL);

            hmacSignEnd = clock();

            //Since we need to compare the code to the "original", copy this into a new variable (off the timer) to then compare against on the timer
            memcpy(fake_original_auth_code, auth_code, SHA256_DIGEST_LENGTH);

            hmacVerifyStart = clock();

            if(memcmp(fake_original_auth_code, auth_code, SHA256_DIGEST_LENGTH) != 0) {
                printf("Unequal\n");
            }

            hmacVerifyEnd = clock();

            hmac_verification_time_total += (double) ((hmacSignEnd - hmacSignStart) + (hmacVerifyEnd - hmacVerifyStart)) / CLOCKS_PER_SEC;

            // free(auth);
        }
    }
}

//dynamic path length, check length in different versions of BGPsec!
//Also measure time to sign
//frequency of anouncements? - Cameron has a paper
int main(int argc, char *argv[]) {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    std::vector<int> num_times{10000};

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

    //Calculate time to crunch verifications and signatures at different byte lengths
    for(unsigned int i = 0; i < num_times.size(); i++) {
        ecdsaLengthFile << num_times[i] << " Iterations," << "ECDSA Signature,ECDSA Verification\n";
        hmacLengthFile << num_times[i] << " Iterations," << "HMAC Signature,HMAC Verification\n";

        for(int path_length = 1; path_length <= 8; path_length++) {
            calculateECDSARuntime(ecdsa_verification_time_total, ecdsa_signature_time_total, num_times[i], key, path_length, hash);
            ecdsaLengthFile << path_length << ", " << ecdsa_signature_time_total << "," << ecdsa_verification_time_total << "\n"; 

            calculateHMACRuntime(hmac_verification_time_total, hmac_signature_time_total, num_times[i], hkey, path_length, fake_original_auth_code);
            hmacLengthFile << path_length << "," << hmac_signature_time_total << "," << hmac_verification_time_total<< "\n";
        }

        ecdsaLengthFile << "\n";
        hmacLengthFile << "\n";
    }

    //cleanup
    EC_KEY_free(key);

    return 0;
}
