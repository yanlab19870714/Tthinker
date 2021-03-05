//########################################################################
//## Copyright 2019 Da Yan http://www.cs.uab.edu/yanda
//##
//## Licensed under the Apache License, Version 2.0 (the "License");
//## you may not use this file except in compliance with the License.
//## You may obtain a copy of the License at
//##
//## //http://www.apache.org/licenses/LICENSE-2.0
//##
//## Unless required by applicable law or agreed to in writing, software
//## distributed under the License is distributed on an "AS IS" BASIS,
//## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//## See the License for the specific language governing permissions and
//## limitations under the License.
//########################################################################

//########################################################################
//## Contributors
//##
//##
//########################################################################

#include "kernel_app.h"
#include <math.h> // used for ceil method, as it won't compile without it on Windows. 
// #include <chrono>
using namespace std::chrono;

int main(int argc, char **argv)
{
    auto start = steady_clock::now();

    if(argc != 8 && argc != 9){
		cout<<"arg1 = input path, arg2 = number of threads"
				<<", arg3 = degree ratio, arg4 = min_size, arg5 = time delay threshold, "
						<<"arg6 = kernel file, arg7 = prime_k, arg8 = use trie check"<<endl;
		return -1;
	}
    char* input_path = argv[1];
    num_compers = atoi(argv[2]); //number of compers
	gdmin_deg_ratio = atof(argv[3]);
	gnmin_size = atoi(argv[4]);
	gnmax_size = INT_MAX;
	gnmin_deg = ceil(gdmin_deg_ratio * (gnmin_size - 1));
	TIME_THRESHOLD = atof(argv[5]);
	kernel_file = argv[6];
	prime_k = atoi(argv[7]);
	if(argc == 9)
		trie_check = atoi(argv[8]);


    QCWorker worker(num_compers);
    worker.load_data(input_path);
    // worker.initialize_tasks();
    worker.run();

    auto end = steady_clock::now();
    float duration = (float)duration_cast<milliseconds>(end - start).count() / 1000;
    cout << "Execution Time:" << duration << endl;

    cout << "trie count: " << trie->print_result() << endl;

    log("Done");
}

//./run ...
