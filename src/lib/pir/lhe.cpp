#include "lhe.h"

// input is number of rows
Matrix LHE::genPublicA(uint64_t m) const {
    Matrix A(m, n);
    random(A);
    return A;
};

// column vector where # of rows is always n
Matrix LHE::sampleSecretKey() const {
    Matrix sk(n, 1);
    random(sk);
    return sk;
};

void LHE::randomPlaintext(Matrix& pt) const {
    random(pt, p);
}

// plaintext has length m, where m is in the parameter used to sample m
// Assuming all elements of pt are less than p;
Matrix LHE::encrypt(const Matrix& A, const Matrix& sk, const Matrix& pt) const {
    if (A.rows != pt.rows) {
        std::cout << "Plaintext dimension mismatch!\n";
        assert(false);
    }

    if (sk.cols != 1 || pt.cols != 1) {
        std::cout << "secret key or plaintext are not column vectors!\n";
        assert(false);
    }

    Matrix ciphertext(A.rows, 1);
    error(ciphertext);
    // constant(&ciphertext); std::cout << "change me back!\n";


    Matrix A_sk = matMulVec(A, sk); 

    matAddInPlace(ciphertext, A_sk);

    // scale plaintext
    Matrix pt_scaled = matMulScalar(pt, Delta);

    matAddInPlace(ciphertext, pt_scaled);

    return ciphertext;
};

Matrix LHE::encryptGivenAs(const Matrix& As, const Matrix& pt) const {
    if (As.rows != pt.rows) {
        std::cout << "Plaintext dimension mismatch!\n";
        assert(false);
    }

    Matrix ciphertext(As.rows, 1);

    double start, end;
    start = currentDateTime();
    error(ciphertext);
    // constant(&ciphertext); std::cout << "change me back!\n";
    end = currentDateTime();
    std::cout << "Encryption error sampling time (ms) " << (end-start) << std::endl;

    matAddInPlace(ciphertext, As);

    // scale plaintext
    Matrix pt_scaled = matMulScalar(pt, Delta);

    matAddInPlace(ciphertext, pt_scaled);

    return ciphertext;
};

// length of ct should match the # of rows of H
Matrix LHE::decrypt(const Matrix& H, const Matrix& sk, const Matrix& ct) const {
    if (H.rows != ct.rows) {
        std::cout << "Ciphertext dimension mismatch!\n";
        assert(false);
    }

    if (sk.cols != 1 || ct.cols != 1) {
        std::cout << "secret key or ciphertext are not column vectors!\n";
        assert(false);
    }

    Matrix H_sk = matMulVec(H, sk);

    Matrix scaled_pt = matSub(ct, H_sk);

    Matrix pt = matDivScalar(scaled_pt, Delta);
    for (size_t i = 0; i < pt.rows; i++) {
        if (pt.data[i] == p) {
            pt.data[i] = 0;
        }
    }

    return pt;
}

// length of ct should match the # of rows of H
Matrix LHE::decryptGivenHs(const Matrix& Hs, const Matrix& sk, const Matrix& ct) const {
    if (Hs.rows != ct.rows) {
        std::cout << "Ciphertext dimension mismatch!\n";
        assert(false);
    }

    if (sk.cols != 1 || ct.cols != 1) {
        std::cout << "secret key or ciphertext are not column vectors!\n";
        assert(false);
    }

    Matrix scaled_pt = matSub(ct, Hs);

    Matrix pt = matDivScalar(scaled_pt, Delta);
    for (size_t i = 0; i < pt.rows; i++) {
        if (pt.data[i] == p) {
            pt.data[i] = 0;
        }
    }

    return pt;
}


