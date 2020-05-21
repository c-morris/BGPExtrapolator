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

//dynamic path length, check length in different versions of BGPsec!
//Also measure time to sign
//frequency of anouncements? - Cameron has a paper
int main(int argc, char *argv[]) {
    char ecdsaFileName[30], hmacFileName[30];

    std::vector<int> num_announcements{50000};
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

    std::ofstream ecdsaFile("ECDSA.csv");
    std::ofstream hmacFile("HMAC.csv");

    //Path length of 1 to 12
    for(int messege_size = 12; messege_size <= 56; messege_size += 4) {
        ecdsaFile << "Number of Verifications (" << messege_size << " bytes),ECDSA Signature,ECDSA Verification\n";
        hmacFile << "Number of Verifications (" << messege_size << " bytes),HMAC Signature,HMAC Verification\n";

        unsigned char messege[messege_size];
        unsigned char hash[SHA256_DIGEST_LENGTH];

        clock_t ecdsaSignStart, ecdsaSignEnd;
        clock_t ecdsaVerifyStart, ecdsaVerifyEnd;

        clock_t hmacSignStart, hmacSignEnd;
        clock_t hmacVerifyStart, hmacVerifyEnd;

        for(int i = 0; i < num_announcements.size(); i++) {
            double ecdsa_verification_time_total = 0;
            double ecdsa_signature_time_total = 0;

            //ECDSA p-256 with SHA-256
            for(unsigned int t = 0; t < num_announcements[i]; t++) {
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

            // printf("Time spent verifying ECDSA p-256 with SHA-256 hashing: %f (seconds)\n", ecdsa_verification_time_total);
            // printf("Average time to verify ECDSA p-256 with SHA-256 hashing: %f (milliseconds)\n\n", ((ecdsa_verification_time_total / (double) NUM_ANNOUNCEMENTS) * 1000.0));
            
            ecdsaFile << num_announcements[i] << "," << ecdsa_verification_time_total << "," << ecdsa_signature_time_total << "\n";

            double hmac_verification_time_total = 0;
            double hmac_signature_time_total = 0;

            unsigned char hkey[HMAC_KEY_LENGTH];
            unsigned char fake_original_auth_code[SHA256_DIGEST_LENGTH];
            for(unsigned int t = 0; t < num_announcements[i]; t++) {
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

            // printf("Total time spent verifying HMAC with SHA-256 hashing: %f (seconds)\n", hmac_verification_time_total);
            // printf("Average time to verify HMAC with SHA-256 hashing: %f (milliseconds)\n", ((hmac_verification_time_total / (double) NUM_ANNOUNCEMENTS) * 1000.0));

            hmacFile << num_announcements[i] << "," << hmac_verification_time_total << "," << hmac_signature_time_total << "\n";
        }

        ecdsaFile << "\n";
        hmacFile << "\n";
    }

    EC_KEY_free(key);

    return 0;
}
