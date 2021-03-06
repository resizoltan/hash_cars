// hash_cars.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <conio.h>
//#include <Windows.h>
#include <string.h>
#include <random>
#include <time.h>
#include <vector>
#include <list>
#include <chrono>
#include <stdlib.h>
#include <algorithm>
#include <thread>

using std::ifstream;
using std::ofstream;
using std::ios;
using std::cout;
using std::cin;
using std::endl;

using std::vector;
using std::list;

using std::string;


int max_bonus_points, max_distance_points, max_score;
int total_bonus_points[4] = { 0 };
int	total_distance_points[4] = { 0 };
int	final_score[4] = { 0 };

int R; //number of rows
int C; //number of columns
int F; //fleet
int N; //number of rides
int B; //bonus
int T; //steps in the simpulation
vector<vector<int>> inRides;
vector<vector<int>> base_vehicles[4];

enum enum_inRides {
	a,
	b,
	x,
	y,
	s,
	f,
	distance
};
enum enum_rides {
	ride_no,
	vehicle
};
enum enum_vehicles {
	ride_no_of_vehicle,
	position_x,
	position_y
};
enum enum_rides_data {
	fitness,
	sigma1,
	sigma2
};


//parameters of evolution
//size of population
#define n 50
//ratio of offsprings to population
#define h 7
//size of generation
int g = n + (h*n);
//number of maximal evolutional steps
#define evo 1000
//maximal number of steps without progress
#define stagnation_threshold 20

//number of threads to work with
#define tt 3

//array for individuums
vector<vector<vector<double>>> population;
vector<vector<vector<double>>> pop[4];
vector<vector<double>> offsprings[4];
//index of fitness
int dataIndex;

std::default_random_engine generator;
std::normal_distribution<double> distribution(0.0, 1.0);

//open file
bool openFile(string filename);

void initPopulation();

void initChromosome(int tid);

void evolution(int generations);

void chooseParents(int thread);

void produceOffspring(vector<vector<double>> &parent1, vector<vector<double>> &parent2, int offspring);

void simulate(int tid);

//bool operator overload for sorting by fitness
struct more_than_fitness
{
	inline bool operator() (const vector<vector<double>>& individuum1, const vector<vector<double>>& individuum2)
	{
		return (individuum1[dataIndex][fitness] > individuum2[dataIndex][fitness]);
	}
};

int main()
{


	if (openFile("c_no_hurry.in")) {
		
		initPopulation();

		cout << endl;

		//simulation of generation
		cout << "generation 0:" << endl;
		for (int i = 0; i < 4; i++) {
			simulate(i);
		}
		std::sort(population.begin(), population.end(), more_than_fitness());
		cout << "Fitness: " << population[0][dataIndex][fitness] / (double)max_score * 100 << "%" << endl;
		evolution(evo);

		/*for (int i = 0; i < n; i++) {
			cout << "Fitness of individuum " << i << ": " << population[i][dataIndex][fitness] / (double)max_score * 100 << "%" << endl;
		}
		cout << endl;*/

		/*final_score = total_distance_points + total_bonus_points;
		cout << endl << "Final score: " << final_score << " points (" << total_distance_points << " distance points and " << total_bonus_points << " bonus points).";
		cout << endl << "Max score: " << max_score << " points (" << max_distance_points << " distance points and " << max_bonus_points << " bonus points).";*/

		cout << "finished" << endl;
	}
	_getch();
    return 0;
}

bool openFile(string filename) {
	//SetCurrentDirectory(L"(D:\Production\Visual Studio 15\hash_cars\Input_files\)");
	ifstream inFile(filename);

	if (!inFile.is_open()) {
		cout << "Could not open file" << endl;
		return false;
	}
	else {
		inFile >> R >> C >> F >> N >> B >> T;

		cout << "Rows	R = " << R << endl << "Columns	C = " << C << endl << "Fleet	F = " << F << endl << "Rides	N = " << N << endl << "Bonus	B = " << B << endl << "Steps	T = " << T << endl;

		//define input rides vector
		for (int i = 0; i < N; i++) {
			vector<int> temp;
			int next_data;
			for (int j = 0; j < 6; j++) {
				inFile >> next_data; temp.push_back(next_data);
			}
			temp.push_back(abs(temp[a] - temp[x]) + abs(temp[b] - temp[y]));
			inRides.push_back(temp);
		}
		inFile.close();
	}

	//calculate the maximum achievable score
	max_bonus_points = B * N;
	for (int i = 0; i < N; i++) {
		max_distance_points += inRides[i][distance];
	}
	max_score = max_distance_points + max_bonus_points;

	return true;
}

void initPopulation() {
	srand(time(NULL));
	dataIndex = N;

	//define vehicles 2d vector
	for (int veh = 0; veh < 4; veh++) {
		for (int i = 0; i < F; i++) {

			vector<int> temp;
			temp.push_back(3);
			temp.push_back(0);
			temp.push_back(0);
			for (int j = 3; j < 3 + N; j++) {
				temp.push_back(-1);
			}
			base_vehicles[veh].push_back(temp);
	}
}
	//initialize population

	/*std::thread t[tt];
	for (int tid = 0; tid < tt; ++tid) {
		t[tid] = std::thread(initChromosome, tid);
	}*/
	initChromosome(0);
	/*for (int tid = 0; tid < 4; ++tid) {
		population.insert(population.end(), pop[tid].begin(), pop[tid].end());
	}*/
	//init offspring
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < N; i++) {
			vector<double> temp;
			temp.push_back(-1);
			temp.push_back(0);
			offsprings[j].push_back(temp);
		}
		double sigma1_temp = (double)F / 10;
		double sigma2_temp = (double)N / 25;
		vector<double> data_temp(1, 0);
		data_temp.push_back(sigma1_temp);
		data_temp.push_back(sigma2_temp);
		offsprings[j].push_back(data_temp);
	}


	
}

void initChromosome(int tid) {
	for (int j = 0; j < g; j = j + 1) {


		//initialize chromosome with random mutation widths, without allocating rides 
		vector<vector<double>> temp_rides;
		for (int i = 0; i < N; i++) {
			vector<double> temp;
			temp.push_back(0);
			temp.push_back(0);
			temp_rides.push_back(temp);
		}
		double sigma1_temp = double(rand() % (F - 1)) + 1;
		double sigma2_temp = double(rand() % (N - 1)) + 1;
		vector<double> data_temp(1, 0);
		data_temp.push_back(sigma1_temp);
		data_temp.push_back(sigma2_temp);
		temp_rides.push_back(data_temp);

		//create a list of numbers for random ride allocation
		std::list<int>::const_iterator iterator;
		list<int> numbers;
		for (int i = 0; i < N; i++) {
			numbers.push_back(i);
		}

		//allocate the rides to the rides indices randomly
		for (int i = 0; i < N; i++) {

			int index = rand() % (N - i);
			iterator = numbers.begin();
			std::advance(iterator, index);
			temp_rides[i][ride_no] = *iterator;
			iterator = numbers.erase(iterator);
			temp_rides[i][vehicle] = rand() % F;
		}

		//add chromosome to starting population
		population.push_back(temp_rides);
	}
}

void simulate(int tid) {

	for (int individuum = tid; individuum < g; individuum = individuum + 4) {
		vector<vector<int>> vehicles(base_vehicles[tid]);
		int temp_vehicle = 0;
		for (int i = 0; i < N; i++) {
			temp_vehicle = population[individuum][i][vehicle];
			vehicles[temp_vehicle].at(vehicles[temp_vehicle].at(ride_no_of_vehicle)) = population[individuum][i][ride_no];
			vehicles[temp_vehicle].at(ride_no_of_vehicle)++;

		}

		total_distance_points[tid] = 0;
		total_bonus_points[tid] = 0;
		final_score[tid] = 0;


		for (int v = 0; v < F; v++) {
			int r = 3;
			int current_x, current_y, start_x, start_y;
			int next_ride;
			int next_distance;
			int time = -1;
			bool startontime;


			while (vehicles[v][r] != -1) {

				current_x = vehicles[v].at(position_x);
				current_y = vehicles[v].at(position_y);
				next_ride = vehicles[v].at(r);
				start_x = inRides[next_ride][a];
				start_y = inRides[next_ride][b];
				next_distance = abs(current_x - start_x) + abs(current_y - start_y);

				int distance_points = 0, bonus_points = 0, points = 0;

				time += next_distance;
				if (time <= inRides[next_ride][s] - 1) {
					time = inRides[next_ride][s] - 1;
					startontime = true;
				}
				else {
					startontime = false;
				}
				int starttime = time + 1;
				time += inRides[next_ride][distance];

				if (time < T) {
					if (startontime) {
						bonus_points = B;
						total_bonus_points[tid] += bonus_points;
					}
					distance_points = inRides[next_ride][distance];
					total_distance_points[tid] += distance_points;
					points = distance_points + bonus_points;
					//cout << "Individuum " << individuum << ", vehicle " << v << ", ride " << next_ride << ", distance points " << distance_points << ", total distance points " << total_distance_points << endl;
				}

				vehicles[v].at(position_x) = inRides[next_ride][x];
				vehicles[v].at(position_y) = inRides[next_ride][y];


				//cout << "Vehicle " << v << " after ride " << next_ride << "(started at t=" << starttime << ") is at position x=" << vehicles[v][position_x] << ", y=" << vehicles[v][position_y]
				//<< ". Points = " << points << "(" << distance_points << " points + " << bonus_points << " bonus points)." << endl;
				r++;
				if (r >= 3 + N) break;
			}

			//cout << endl;
		}
		final_score[tid] = total_distance_points[tid] + total_bonus_points[tid];
		population[individuum][dataIndex][fitness] = final_score[tid];
		/*cout << endl << "Final score: " << final_score << " points (" << total_distance_points << " distance points and " << total_bonus_points << " bonus points).";
		cout << endl << "Max score: " << max_score << " points (" << max_distance_points << " distance points and " << max_bonus_points << " bonus points).";
		cout << endl << "Efficiency: " << (double)final_score / max_score * 100 << "% (" << (double)total_distance_points / max_distance_points * 100 << "% of distance and " << (double)total_bonus_points / max_bonus_points * 100 << "% of bonus)." << endl << endl << endl;
		*/
	}


	/*for (int i = 0; i < n; i++) {
		cout << "Fitness of individuum " << i << ": " << population[i][dataIndex][fitness] / (double)max_score * 100 << "%" << endl;
	}
	cout << endl;*/
}

int no_progress_since;
double prev_fitness, cur_fitness;

void evolution(int generations){

	for (int generation = 0; generation < generations; generation++) {
		auto start = std::chrono::system_clock::now();
		prev_fitness = population[0][dataIndex][fitness];

		/*for (int production_index = n; production_index < g; production_index = production_index + 1) {
			//choose parents
			int index1 = rand() % n;
			int index2;
			vector<vector<double>> parent1 = population[index1];
			do {
				index2 = rand() % n;
			} while (index1 = index2);
			vector<vector<double>> parent2 = population[index2];

			produceOffspring(parent1, parent2, 0);
			//population[production_index] = offsprings[0];
		}*/


		std::thread t[tt];
		for (int thread_no = 0; thread_no < tt; ++thread_no) {

			t[thread_no] = std::thread(chooseParents, thread_no);
		}
		chooseParents(3);

		for (int thread_no = 0; thread_no < tt; ++thread_no) {
			t[thread_no].join();
		}

		/*std::thread t1 = std::thread(chooseParents, 0);
		std::thread t2 = std::thread(chooseParents, 1);
		t1.join();
		t2.join();*/

		cout << endl << "generation " << generation +1<< ":" << endl;
		for (int thread_no = 0; thread_no < tt; ++thread_no) {

			t[thread_no] = std::thread(simulate, thread_no);
		}
		simulate(3);

		for (int thread_no = 0; thread_no < tt; ++thread_no) {
			t[thread_no].join();
		}

		//sort generation
		std::sort(population.begin(), population.end(), more_than_fitness());
		cout << "Fitness: " << population[0][dataIndex][fitness] / (double)max_score * 100 << "%" << endl;


		cur_fitness = population[0][dataIndex][fitness];
		if (cur_fitness <= prev_fitness) {
			no_progress_since++;
			if (no_progress_since >= stagnation_threshold) break;
		}
		else {
			no_progress_since = 0;
		}
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		cout << "Elapsed seconds: " << elapsed_seconds.count() << endl;
	}

}

void produceOffspring(vector<vector<double>> &parent1, vector<vector<double>> &parent2, int tid) {

	//initialize offspring chromosome without mutation widths, without allocating rides 
	//cout << "offspring in production" << endl;
	//auto start = std::chrono::system_clock::now();
	for (int i = 0; i < N; i++) {
		vector<double> temp;
		temp.push_back(-1);
		temp.push_back(0);
		offsprings[tid][i] = temp;
	}


	//----------------intermediary crossover of mutation widths---------------------------------------------
	double* temp_sigma1;
	double* temp_sigma2;

	do {
		__int64 mask_pos_1 = 0;
		for (int i = 0; i < sizeof(double) * 8; i++) {
			mask_pos_1 += (rand() % 2) << i;
		}
		__int64 mask_neg_1 = ~mask_pos_1;

		__int64* p11 = reinterpret_cast<__int64*>(&parent1[dataIndex][sigma1]);
		__int64* p21 = reinterpret_cast<__int64*>(&parent2[dataIndex][sigma1]);


		//produce mutation widths of offspring
		__int64 o1 = (mask_pos_1 & *p11) | (mask_neg_1 & *p21);


		temp_sigma1 = reinterpret_cast<double*>(&o1);


	} while ((*temp_sigma1 == 0));

	do {


		__int64 mask_pos_2 = 0;
		for (int i = 0; i < sizeof(double) * 8; i++) {
			mask_pos_2 += (rand() % 2) << i;
		}
		__int64 mask_neg_2 = ~mask_pos_2;


		__int64* p12 = reinterpret_cast<__int64*>(&parent1[dataIndex][sigma2]);
		__int64* p22 = reinterpret_cast<__int64*>(&parent2[dataIndex][sigma2]);

		//produce mutation widths of offspring
		__int64 o2 = (mask_pos_2 & *p12) | (mask_neg_2 & *p22);

		temp_sigma2 = reinterpret_cast<double*>(&o2);

	} while ((*temp_sigma2 == 0));
	//cout << repetitions2 << endl;
	//*temp_sigma1 = parent1[dataIndex][sigma1];
	//--------------mutation of mutation widths-----------------------------------------------------------

	*temp_sigma1 = *temp_sigma1 * exp(distribution(generator));
	*temp_sigma2 = *temp_sigma2 * exp(distribution(generator));

	//cout << "parent1 sigma1: " << parent1[dataIndex][sigma1] << endl << "parent1 sigma2: " << parent2[dataIndex][sigma2] << endl << "offspring sigma1: " << *temp_sigma2 << endl;

	//--------------discrete recombination of decision variables------------------------------------------
	//auto end = std::chrono::system_clock::now();
	//std::chrono::duration<double> elapsed_seconds = end - start;
	//cout << elapsed_seconds.count() << endl;
	//cout << "recombination of chromosomes" << endl;
	//start = std::chrono::system_clock::now();
	//randomly select indices for parent1
	vector<int> ox_indices;
	for (int i = 0; i < N; i++) {
		if (rand() % 2) {
			ox_indices.push_back(i);
		}
	}
	ox_indices.push_back(-1);

	vector<bool> used_ride(N, false);

	//copy values at the indices from parent1 to offspring
	for (int i = 0; i < ox_indices.size() - 1; i++) {
		offsprings[tid][ox_indices[i]] = parent1[ox_indices[i]];
		used_ride[parent1[ox_indices[i]][ride_no]] = true;
	}

	//cout << "parent2" << endl;

	int iter = 0;
	for (int i = 0; i < N; i++) {
		iter = 0;
		while (offsprings[tid][i][ride_no] == -1  && iter < N) {
			if (!used_ride[parent2[iter][ride_no]]) {
				offsprings[tid][i] = parent2[iter];
			}
			iter++;
		}
	}

	//copy not yet present values from parent2 to offspring
	/*int iter = 0;
	for (int i = 0; i < N; i++) {
		while (offsprings[offspring][i][ride_no] == -1) {
			bool already_present = 0;
			for (int j = 0; j < ox_indices.size() - 1; j++) {
				if (parent2[iter][ride_no] == parent1[ox_indices[j]][ride_no]) {
					already_present = true;
					break;
				}
			}
			if (!already_present)
			{
				offsprings[offspring][i]= parent2[iter];
			}
			iter++;
		}
	} */
	/*end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	cout << elapsed_seconds.count() << endl;*/
	//cout << "recombination finished" << endl;

	//------------------mutation of offspring-------------------------------------------------------------

	std::normal_distribution<double> dist1(0.0, F/10);
	std::normal_distribution<double> dist2(0.0, N/100);

	for (int i = 0; i < N; i++) {
		int new_vehicle = offsprings[tid][i][vehicle] + int(dist1(generator));
		if (new_vehicle < 0) {
			offsprings[tid][i][vehicle] = 0;
		} 
		else if (new_vehicle >= F) {
			offsprings[tid][i][vehicle] = F - 1;
		}
		else {
			offsprings[tid][i][vehicle] = new_vehicle;
		}
	}

	for (int i = 0; i < N; i++) {
		int offset = int(dist2(generator));
		if ((i + offset) >= 0 && (i + offset) < N) {
			vector<double> temp_offspring = offsprings[tid][i]; offsprings[tid][i] = offsprings[tid][i + offset]; offsprings[tid][i + offset] = temp_offspring;
		}
	}

	vector<double> data_temp(1, 0);
	data_temp.push_back(*temp_sigma1);
	data_temp.push_back(*temp_sigma2);
	tid;
	offsprings[tid][N] = data_temp;

	/*for (int i = 0; i < N; i++) {
		cout << "ride_no: " << offspring[i][ride_no] << "\t vehicle:" << offspring[i][vehicle] << endl;
	}*/
	//cout << "offspring finished" << endl;
	/*auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	cout << elapsed_seconds.count() << endl;*/
};

void chooseParents(int thread) {
	for (int production_index = n + thread; production_index < g; production_index = production_index + 1) {
		//choose parents
		int index1 = rand() % n;
		int index2;
		vector<vector<double>> parent1 = population[index1];
		do {
			index2 = rand() % n;
		} while (index1 == index2);
		vector<vector<double>> parent2 = population[index2];

		//production of offspring
		//cout << endl  << "Offspring " << production_index - n << ",\tparents " << index1 << ",\t" << index2 << endl;
		produceOffspring(parent1, parent2, thread);
		population[production_index] = offsprings[thread];
	}
}