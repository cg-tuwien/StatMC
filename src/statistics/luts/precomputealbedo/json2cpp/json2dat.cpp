// © 2024-2025 Hiroyuki Sakai

#include <float.h>      // LDBL_DIG

#include <iomanip>      // std::setprecision
#include <iostream>     // std::cout, std::endl
#include <sstream>      // std::stringstream
#include <string>       // std::string

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#ifdef LDBL_DECIMAL_DIG
    #define OP_LDBL_DECIMAL_DIG (LDBL_DECIMAL_DIG)
#else  
    #ifdef DECIMAL_DIG
        #define OP_LDBL_DECIMAL_DIG (DECIMAL_DIG)
    #else  
        #define OP_LDBL_DECIMAL_DIG (LDBL_DIG + 3)
    #endif
#endif

using namespace rapidjson;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Please specify a file." << std::endl;
        return 1;
    }

    std::cout << std::setprecision(OP_LDBL_DECIMAL_DIG); // Show enough digits for long double

    FILE* fp = fopen(argv[1], "r"); // For non-Windows use "r"

    char buffer[65536];
    FileReadStream is(fp, buffer, sizeof(buffer));

    Document d;
    d.ParseStream(is);

    fclose(fp);

    if (d.HasParseError()) {
        std::cout << "Parse error " << d.GetParseError() << " at offset " << d.GetErrorOffset() << "encountered.\n";
        return 1;
    }


    const Value &results = d["results"];    
    assert(results.IsArray());

    const unsigned int nDims = d["nDims"].GetInt();

    unsigned int* indices       = (unsigned int*) alloca(sizeof(unsigned int) * nDims);
    unsigned int* lengths       = (unsigned int*) alloca(sizeof(unsigned int) * nDims);

    for (unsigned int i = 0; i < nDims; i++) {
        indices[i] = 0;
        lengths[i] = d["lengths"][i].GetInt();
    }

    unsigned int dimIndex = 0;

    for (SizeType i = 0; i < results.Size(); i++) {
        std::stringstream indexStream;
        for (int j = nDims - 1; j >= 0; j--)
            indexStream << indices[j] << " ";
        std::string indexString = indexStream.str();

        std::cout << indexString << results[i]["albedo"].GetDouble() << std::endl;

        indices[0]++;

        while (indices[dimIndex] == lengths[dimIndex]) {
            if (dimIndex == nDims - 1)
                goto breakAll;

            indices[dimIndex++] = 0;
            indices[dimIndex]++;
        }

        dimIndex = 0;
    }

    breakAll:

    return 0;
}
