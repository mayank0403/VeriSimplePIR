#include "pir/preproc_pir.h"
#include <map>
#include "../global_parameters.h"

void benchmark_verisimplepir_offline_server_compute(const VeriSimplePIR& pir, const bool verbose = false) {

    double start, end;

    // Matrix D = pir.db.packDataInMatrix(pir.dbParams);
    std::cout << "sampling random db matrix...\n";
    Matrix D_T(pir.dbParams.m, pir.dbParams.ell);
    random_fast(D_T, pir.dbParams.p);
    std::cout << "sampled database.\n";

    const uint64_t index = 1;

    unsigned char preproc_hash[SHA256_DIGEST_LENGTH];
    // pir.HashAandH(preproc_hash, A_2, H_2);

    // Offline phase
    uint64_t iters = 1;

    std::cout << "generating fake client message...\n";
    const auto preproc_ct_sk_pair = pir.PreprocFakeClientMessage();

    const auto preproc_cts = std::get<0>(preproc_ct_sk_pair);
    const auto preproc_sks = std::get<1>(preproc_ct_sk_pair);

    std::cout << "beginning server compute benchmarks\n";

    std::vector<Multi_Limb_Matrix> preproc_res_cts;  //  = pir.PreprocAnswer(preproc_cts, D_T);
    start = currentDateTime();
    for (uint64_t i = 0; i < iters; i++) {
        preproc_res_cts = pir.PreprocAnswer(preproc_cts, D_T);
    }
    end = currentDateTime();
    std::cout << "Preproc answer generation time: " << (end-start)/iters << " ms\n";

    Matrix preproc_Z;
    start = currentDateTime();
    for (uint64_t i = 0; i < iters; i++) {
        preproc_Z = pir.PreprocProve(preproc_hash, preproc_cts, preproc_res_cts, D_T);
    }
    end = currentDateTime();
    std::cout << "Preproc proof generation time: " << (end-start)/iters << " ms\n";

    std::cout << std::endl;
}

void benchmark_verisimplepir_offline_client_compute(const VeriSimplePIR& pir, const bool verbose = false) {

    double start, end;

    const uint64_t index = 1;

    // std::cout << "sampling random A matrix...\n";
    Matrix A_1 = pir.FakeInit();
    Multi_Limb_Matrix A_2 = pir.PreprocFakeInit();

    Matrix H_1 = pir.GenerateFakeHint();
    Multi_Limb_Matrix H_2 = pir.PreprocGenerateFakeHint();

    unsigned char preproc_hash[SHA256_DIGEST_LENGTH];
    // pir.HashAandH(preproc_hash, A_2, H_2);

    // Offline phase
    uint64_t iters = 1;

    const BinaryMatrix C = pir.PreprocSampleC();

    std::cout << "generating client message...\n";
    const auto preproc_ct_sk_pair = pir.PreprocClientMessage(A_2, C);
    start = currentDateTime();
    for (uint64_t i = 0; i < iters; i++) {
        pir.PreprocClientMessage(A_2, C);
    }
    end = currentDateTime();
    std::cout << "Client message generation time: " << (end-start)/iters << " ms\n";

    const auto preproc_cts = std::get<0>(preproc_ct_sk_pair);
    const auto preproc_sks = std::get<1>(preproc_ct_sk_pair);

    const auto preproc_res_cts = pir.PreprocFakeAnswer();
    const auto preproc_Z = pir.PreprocFakeProve();

    const bool fake = true;
    start = currentDateTime();
    for (uint64_t i = 0; i < iters; i++) {
        pir.PreprocVerify(A_2, H_2, preproc_hash, preproc_cts, preproc_res_cts, preproc_Z, fake);
    }
    end = currentDateTime();
    std::cout << "Preproc verification time: " << (end-start)/iters << " ms\n";

    const Matrix Z = pir.PreprocRecoverZ(H_2, preproc_sks, preproc_res_cts);
    start = currentDateTime();
    for (uint64_t i = 0; i < iters; i++) {
        pir.PreprocRecoverZ(H_2, preproc_sks, preproc_res_cts);
    }
    end = currentDateTime();
    std::cout << "Preproc decryption time: " << (end-start)/iters << " ms\n";

    start = currentDateTime();
    for (uint64_t i = 0; i < iters; i++) {
        pir.VerifyPreprocZ(Z, A_1, C, H_1, fake);
    }
    end = currentDateTime();
    std::cout << "Precompute proof check time: " << (end-start)/iters << " ms\n";

    std::cout << std::endl;
}

void benchmark_verisimplepir_online(const VeriSimplePIR& pir, const bool verbose = false) {

    double start, end;

    std::cout << "database params: "; pir.dbParams.print();
    std::cout << "sampling random db matrix...\n";

    PackedMatrix D_packed = packMatrixHardCoded(pir.dbParams.ell, pir.dbParams.m, pir.dbParams.p);

    const uint64_t index = 1;

    Matrix H = pir.GenerateFakeHint();
    std::cout << "offline download size = " << H.rows*H.cols*sizeof(Elem) / (1ULL << 20) << " MiB\n";

    auto C_and_Z = pir.SampleFakeCandZ();

    const BinaryMatrix C = std::get<0>(C_and_Z);
    const Matrix Z = std::get<1>(C_and_Z);

    std::cout << "sampling random A matrix...\n";
    start = currentDateTime();
    Matrix A = pir.Init();
    end = currentDateTime();
    double a_exp_time = (end-start);
    std::cout << "A expansion time: " << a_exp_time << " ms | " <<  a_exp_time/1000 << " sec" << std::endl;

    double start_prepare, end_prepare;
    start_prepare = currentDateTime();
    Matrix sk = pir.GetSk();
    start = currentDateTime();
    Matrix As = matMulVec(A, sk);
    end = currentDateTime();
    double as_time = (end-start);
    std::cout << "Prepare As time: " << as_time << " ms | " <<  as_time/1000 << " sec" << std::endl;
    start = currentDateTime();
    Matrix H_sk = matMulVec(H, sk);
    end = currentDateTime();
    double hs_time = (end-start);
    std::cout << "Prepare Hs time: " << hs_time << " ms | " <<  hs_time/1000 << " sec" << std::endl;
    end_prepare = currentDateTime();
    double prepare_time = (end_prepare-start_prepare);
    std::cout << "Total Prepare time: " << prepare_time << " ms | " <<  prepare_time/1000 << " sec" << std::endl;

    start = currentDateTime();
    auto ct = pir.QueryGivenAs(As, index);
    end = currentDateTime();
    double query_time = (end-start);
    std::cout << "Query generation time (given As): " << query_time << " ms | " <<  query_time/1000 << " sec" << std::endl;

    start = currentDateTime();
    Matrix ans = pir.Answer(ct, D_packed);
    end = currentDateTime();
    double answer_time = (end-start);
    std::cout << "Answer generation time: " << answer_time << " ms | " <<  answer_time/1000 << " sec" << std::endl;

    start = currentDateTime();
    pir.FakePreVerify(ct, ans, Z, C);
    end = currentDateTime();
    double verify_time = (end-start);
    std::cout << "Verification time: " << verify_time << " ms | " <<  verify_time/1000 << " sec" << std::endl;

    entry_t res;
    start = currentDateTime();
    res = pir.RecoverGivenHs(H_sk, ans, sk, index);
    end = currentDateTime();
    double recovery_time = (end-start);
    std::cout << "Recovery time: " << recovery_time << " ms | " <<  recovery_time/1000 << " sec" << std::endl;

    //double total_time = query_time + answer_time + verify_time + recovery_time;
    //std::cout << "Total time: " << total_time << " ms | " <<  total_time/1000 << " sec" << std::endl;
}

// All recorded metrics
//#define HINT_MB "Hints (MiB)"
//#define ONLINE_STATE_KB "Online State (KiB)"
//#define OFFLINE_UP_KB "Offline Up (KiB)"
//#define OFFLINE_DOWN_KB "Offline Down (KiB)"
//#define PREPARE_UP_KB "Prep Up (Kib)"
//#define PREPARE_DOWN_KB "Prep Down (KiB)"
//#define QUERY_UP_KB "Query Up (KiB)"
//#define QUERY_DOWN_KB "Query Down (KiB)"
//#define GLOBAL_PREPR_S "Global Prepr (S)"
#define SV_CLIENT_SPEC_PREPR_S "Server Per-Client Prepr (s)"
#define CLIENT_PREPR_S "Client Local Prepr (s)"
#define CLIENT_PREP_PRE_S "Client Prepa Pre Req (s)"
#define SERVER_PREP_S "Server Prepa Comp (s)"
#define CLIENT_PREP_POST_S "Client Prepa Post Req (s)"
#define CLIENT_Q_REQ_GEN_MS "Query: Client Req Gen (ms)"
#define SERVER_Q_RESP_S "Query: Server Comp (s)"
#define CLIENT_Q_DEC_MS "Query: Client Decryption (ms)"
#define CLIENT_Q_VER_MS "Query: Client Verification (ms)"
#define VSPIR_UNCOMP_Z "VSPIR Uncompr. Z (KiB)"

void benchmark_verisimplepir_full(const VeriSimplePIR& pir, const bool verbose = false) {

    // Important metrics
    map<string, double> dict;
    //dict[HINT_MB] = 0; // mb is MiB, kb is KiB, s is sec, ms is ms
    //dict[ONLINE_STATE_KB] = 0;
    //dict[OFFLINE_UP_KB] = 0;
    //dict[OFFLINE_DOWN_KB] = 0;
    //dict[PREPARE_UP_KB] = 0;
    //dict[PREPARE_DOWN_KB] = 0;
    //dict[QUERY_UP_KB] = 0;
    //dict[QUERY_DOWN_KB] = 0;
    //dict[GLOBAL_PREPR_S] = 0;
    dict[SV_CLIENT_SPEC_PREPR_S] = 0;
    dict[CLIENT_PREPR_S] = 0;
    dict[CLIENT_PREP_PRE_S] = 0;
    dict[SERVER_PREP_S] = 0;
    dict[CLIENT_PREP_POST_S] = 0;
    dict[CLIENT_Q_REQ_GEN_MS] = 0;
    dict[SERVER_Q_RESP_S] = 0;
    dict[CLIENT_Q_DEC_MS] = 0;
    dict[CLIENT_Q_VER_MS] = 0;
    dict[VSPIR_UNCOMP_Z] = 0;

    double start, end;

    std::cout << "database params: "; pir.dbParams.print();
    std::cout << "sampling random db matrix...\n";

    const PackedMatrix D_packed = packMatrixHardCoded(pir.dbParams.ell, pir.dbParams.m, pir.dbParams.p);
    std::cout << "sampling random db matrix...\n";
    Matrix D_T(pir.dbParams.m, pir.dbParams.ell);
    random_fast(D_T, pir.dbParams.p);
    //Matrix D(pir.dbParams.ell, pir.dbParams.m);
    //random_fast(D, pir.dbParams.p);
    std::cout << "sampled databases.\n";

    unsigned char preproc_hash[SHA256_DIGEST_LENGTH];

    const uint64_t index = 1;

    start = currentDateTime();
    Matrix A_1 = pir.Init();
    Multi_Limb_Matrix A_2 = pir.PreprocInit();
    end = currentDateTime();
    std::cout << "Global preprocessing A1, A2 expansion time: " << (end-start) << " ms\n";
    dict[CLIENT_PREPR_S] += (end-start)/1000;

    start = currentDateTime();
    Matrix H = pir.GenerateFakeHint();
    //Matrix H = pir.GenerateHintPackedIn(A_1, D_packed);
    std::cout << "Hint size = " << H.rows*H.cols*sizeof(Elem) / (1ULL << 20) << " MiB\n";
    Multi_Limb_Matrix H_2 = pir.PreprocGenerateFakeHint();
    //Multi_Limb_Matrix H_2 = pir.PreprocGenerateHint(A_2, D_T);
    end = currentDateTime();
    std::cout << "Global preprocessing H1, H2 generation time: " << (end-start) << " ms\n";
    std::cout << "Hint (H for offline D^TC^T) main limb size = " << H_2.q_data.rows*H_2.q_data.cols*sizeof(Elem) / (1ULL << 20) << " MiB\n";
    double log_kappa_bytes = std::log2(pir.preproc_lhe.kappa) / 8.0;
    if (log_kappa_bytes < 1.0/8.0) {
        log_kappa_bytes = 1.0/8.0;
    }
    std::cout << "Hint (H for offline D^TC^T) kappa limb size = " << H_2.kappa_data.rows*H_2.kappa_data.cols*log_kappa_bytes / (1ULL << 20) << " MiB\n";

    const BinaryMatrix C = pir.PreprocSampleC();

    std::cout << "generating client message...\n";
    start = currentDateTime();
    const auto preproc_ct_sk_pair = pir.PreprocClientMessage(A_2, C);
    end = currentDateTime();
    std::cout << "Offline client message generation time: " << (end-start) << " ms\n";
    dict[CLIENT_PREPR_S] += (end-start)/1000;

    const auto preproc_cts = std::get<0>(preproc_ct_sk_pair);
    const auto preproc_sks = std::get<1>(preproc_ct_sk_pair);

    std::cout << "beginning server compute benchmarks\n";

    std::vector<Multi_Limb_Matrix> preproc_res_cts;  //  = pir.PreprocAnswer(preproc_cts, D_T);
    start = currentDateTime();
    preproc_res_cts = pir.PreprocAnswer(preproc_cts, D_T);
    end = currentDateTime();
    std::cout << "Preproc answer generation time: " << (end-start) << " ms\n";
    dict[SV_CLIENT_SPEC_PREPR_S] += (end-start)/1000;

    Matrix preproc_Z;
    start = currentDateTime();
    preproc_Z = pir.PreprocProve(preproc_hash, preproc_cts, preproc_res_cts, D_T);
    end = currentDateTime();
    std::cout << "Preproc proof generation time: " << (end-start) << " ms\n";
    dict[SV_CLIENT_SPEC_PREPR_S] += (end-start)/1000;
    
    std::cout << "Preprocessing phase larger Z (proof) size = " << preproc_Z.rows*preproc_Z.cols*sizeof(Elem) / (1ULL << 10) << " KiB\n";

    const bool fake = true;
    start = currentDateTime();
    pir.PreprocVerify(A_2, H_2, preproc_hash, preproc_cts, preproc_res_cts, preproc_Z, fake);
    end = currentDateTime();
    std::cout << "Preproc verification time: " << (end-start) << " ms\n";
    dict[CLIENT_PREPR_S] += (end-start)/1000;

    start = currentDateTime();
    const Matrix Z = pir.PreprocRecoverZ(H_2, preproc_sks, preproc_res_cts);
    end = currentDateTime();
    std::cout << "Preproc Z decryption time: " << (end-start) << " ms\n";
    dict[CLIENT_PREPR_S] += (end-start)/1000;

    std::cout << "Online phase Z (proof) size = " << Z.rows*Z.cols*sizeof(Elem) / (1ULL << 10) << " KiB\n";
    dict[VSPIR_UNCOMP_Z] += Z.rows*Z.cols*sizeof(Elem) / (1ULL << 10);

    start = currentDateTime();
    pir.VerifyPreprocZ(Z, A_1, C, H, fake);
    end = currentDateTime();
    std::cout << "Offline proof verification time: " << (end-start) << " ms\n";
    dict[CLIENT_PREPR_S] += (end-start)/1000;

    std::cout << "sampling random A matrix...\n";
    start = currentDateTime();
    Matrix A = pir.Init();
    end = currentDateTime();
    double a_exp_time = (end-start);
    std::cout << "A expansion time: " << a_exp_time << " ms | " <<  a_exp_time/1000 << " sec" << std::endl;
    dict[CLIENT_PREP_PRE_S] += a_exp_time/1000;

    double start_prepare, end_prepare;
    start_prepare = currentDateTime();
    
    start = currentDateTime();
    Matrix sk = pir.GetSk();
    Matrix As = matMulVec(A, sk);
    end = currentDateTime();
    double as_time = (end-start);
    std::cout << "Prepare As time: " << as_time << " ms | " <<  as_time/1000 << " sec" << std::endl;
    dict[CLIENT_PREP_PRE_S] += as_time/1000;

    start = currentDateTime();
    Matrix H_sk = matMulVec(H, sk);
    end = currentDateTime();
    double hs_time = (end-start);
    std::cout << "Prepare Hs time: " << hs_time << " ms | " <<  hs_time/1000 << " sec" << std::endl;
    dict[CLIENT_PREP_PRE_S] += hs_time/1000;

    end_prepare = currentDateTime();
    double prepare_time = (end_prepare-start_prepare);
    std::cout << "Total Prepare time: " << prepare_time << " ms | " <<  prepare_time/1000 << " sec" << std::endl;

    start = currentDateTime();
    auto ct = pir.QueryGivenAs(As, index);
    end = currentDateTime();
    double query_time = (end-start);
    std::cout << "Query generation time (given As): " << query_time << " ms | " <<  query_time/1000 << " sec" << std::endl;
    dict[CLIENT_Q_REQ_GEN_MS] += query_time;

    start = currentDateTime();
    Matrix ans = pir.Answer(ct, D_packed);
    end = currentDateTime();
    double answer_time = (end-start);
    std::cout << "Answer generation time: " << answer_time << " ms | " <<  answer_time/1000 << " sec" << std::endl;
    dict[SERVER_Q_RESP_S] += answer_time/1000;

    start = currentDateTime();
    pir.FakePreVerify(ct, ans, Z, C);
    end = currentDateTime();
    double verify_time = (end-start);
    std::cout << "Verification time: " << verify_time << " ms | " <<  verify_time/1000 << " sec" << std::endl;
    dict[CLIENT_Q_VER_MS] += verify_time;

    entry_t res;
    start = currentDateTime();
    res = pir.RecoverGivenHs(H_sk, ans, sk, index);
    end = currentDateTime();
    double recovery_time = (end-start);
    std::cout << "Recovery time: " << recovery_time << " ms | " <<  recovery_time/1000 << " sec" << std::endl;
    dict[CLIENT_Q_DEC_MS] += recovery_time;

    //double total_time = query_time + answer_time + verify_time + recovery_time;
    //std::cout << "Total time: " << total_time << " ms | " <<  total_time/1000 << " sec" << std::endl;
    std::cout << "----------------------------\n" << std::endl; 
    for (auto it : dict) {
    std::cout << it.first << " : " << it.second << std::endl;
  }
}

int main() {
    // const uint64_t N = 1ULL<<30;
    // const uint64_t N = 1ULL<<33;
    const uint64_t N = (1ULL<<N_VALUE); // @AGAM NOTE NOTE NOTE: Change BASIS every time
    // const uint64_t N = 8*(1ULL<<33);
    // const uint64_t N = 16*(1ULL<<33);
    // const uint64_t N = 1ULL<<35;
    // const uint64_t d = 2048;
    // const uint64_t d = 128;
    const uint64_t d = D_VALUE;

    // const bool verbose = true;
    const bool verbose = false;

    std::cout << "Input params: N = " << N << " d = " << d << std::endl;
    std::cout << "database size: " << N*d / (8.0*(1ULL<<20)) << " MiB\n";

    const bool honest_hint = true;
    // const bool honest_hint = false;
    VeriSimplePIR pir(
        N, d, 
        true,   // allowTrivial
        verbose, 
        false,  // SimplePIR
        false,  // random data
        1,   // batch size
        true,  // preprocessed
        honest_hint);

    if (honest_hint)
        std::cout << "\n\n     <---- Honest Hint ------>       \n\n\n";

    assert(honest_hint);

    std::cout << "database params: "; pir.dbParams.print();

    // benchmark_verisimplepir_offline_server_compute(pir, verbose);
    // benchmark_verisimplepir_offline_client_compute(pir, verbose);
    //benchmark_verisimplepir_online(pir, verbose);
    benchmark_verisimplepir_full(pir, verbose);
}