#include "pir/pir.h"

int main() {

    std::vector<std::vector<uint64_t>> N_d_params;

    //N_d_params.push_back(std::vector<uint64_t>{(1ULL << 30), 8});
    N_d_params.push_back(std::vector<uint64_t>{(1ULL << 34), 8});

    //N_d_params.push_back(std::vector<uint64_t>{(1ULL << 36), 1});
    // N_d_params.push_back(std::vector<uint64_t>{(1ULL << 25), 2048});

    //N_d_params.push_back(std::vector<uint64_t>{(1ULL << 37), 1});
    // N_d_params.push_back(std::vector<uint64_t>{(1ULL << 26), 2048});

    //N_d_params.push_back(std::vector<uint64_t>{(1ULL << 38), 1});
    //N_d_params.push_back(std::vector<uint64_t>{(1ULL << 39), 1});
    //N_d_params.push_back(std::vector<uint64_t>{(1ULL << 40), 1});
    //N_d_params.push_back(std::vector<uint64_t>{(1ULL << 41), 1});


    for (std::vector<uint64_t> N_d : N_d_params) {
        const uint64_t N = N_d[0];
        const uint64_t d = N_d[1];

        Database db(N, d);

        const bool allowTrivial = false;  // set to true to allow digests that are larger than the database
        const bool verbose      = false;  // set to true to output internals of parameter optimization script
        const bool simplePIR    = false;  // set to true to compute SimplePIR parameters. preproc cannot also be true
        const bool preproc      = true;   // set to true to compute VeriSimplePIR parameters. Set to false to compute VLHE parameters.
        const bool honesthint   = true;  // set to true to activate the honest hint assumption.

        const PlaintextDBParams dbParams = db.computeParams(allowTrivial, verbose, simplePIR, 1, preproc, honesthint);


        std::cout << "N = " << N << " = 2^" << log2(N) << ", d = " << d << std::endl;
        std::cout << "db size = " << double(N*d) / double(8*(1ULL << 30)) << " GiB" << std::endl;
        dbParams.print();
        std::cout << "logq = " << LHE::logq << std::endl; 

        const uint64_t hintSize_bits = LHE::logq * LHE::n * dbParams.ell;

        const uint64_t uploadSize_bits = LHE::logq * dbParams.m;
        uint64_t downloadSize_bits = LHE::logq * dbParams.ell;

        std::cout << "\tOffline Phase\n";

       if (preproc) {
            const uint64_t kappa = dbParams.ell * (Elem)std::ceil(std::sqrt((((double) dbParams.ell) / ((double) dbParams.m))));
            const double log_kappa = log2(kappa);
            const uint64_t additional_hint_size = (LHE::logq + log_kappa) * dbParams.m * LHE::n;
            const uint64_t Z_t_size = log2(dbParams.p*dbParams.m)*dbParams.ell*STAT_SEC_PARAM;
            std::cout << "\t Hints (MiB): hint download = " << double(hintSize_bits + additional_hint_size + Z_t_size)/double(8 * (1ULL << 20)) << " MiB\n";
            const uint64_t ciphertext_upload_size = (LHE::logq + log_kappa) * dbParams.ell * STAT_SEC_PARAM;
            const uint64_t ciphertext_download_size = (LHE::logq + log_kappa) * dbParams.m * STAT_SEC_PARAM;
            const uint64_t preproc_proof_size = log2(dbParams.p*dbParams.m)*dbParams.ell*STAT_SEC_PARAM;
            std::cout << "\t Offline Up (KiB): preproc upload = " << double(ciphertext_upload_size) / double(8 * (1ULL << 10)) << " KiB\n";
            std::cout << "\t Offline Down (KiB): preproc download  = " << double(ciphertext_download_size + preproc_proof_size) / double(8 * (1ULL << 10)) << " KiB\n";
        } else {
            std::cout << "\t hint download = " << double(hintSize_bits)/double(8 * (1ULL << 30)) << " GiB\n";
        }

        std::cout << "\n\tOnline Phase\n";

        std::cout << "\t Online State (KiB): client storage = " << double(hintSize_bits)/double(8 * (1ULL << 10)) << " KiB | " << double(hintSize_bits)/double(8 * (1ULL << 20)) << " MiB" << std::endl;
        std::cout << "\t Query Up (KiB): online upload size = " << double(uploadSize_bits)/double(8*(1ULL << 10)) << " KiB" << std::endl; 
         if (!preproc && !simplePIR) {
            std::cout << "\t Query Down (KiB): online answer size = " << double(downloadSize_bits)/double(8*(1ULL << 10)) << " KiB" << std::endl; 
            const uint64_t proofSize_bits = log2(dbParams.p*dbParams.ell)*dbParams.m*STAT_SEC_PARAM;
            std::cout << "\t proof size = " << double(proofSize_bits)/double(8*(1ULL << 10)) << " KiB" << std::endl; 
        } else {
            std::cout << "\t Query Down (KiB): online download size = " << double(downloadSize_bits)/double(8*(1ULL << 10)) << " KiB" << std::endl; 
        }

        std::cout << std::endl;  
    } 
}