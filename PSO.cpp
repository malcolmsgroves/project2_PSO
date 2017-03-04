#include "PSO.h"

// constans
double BIG_DOUBLE = 9999999999;
double PI = 3.1415926535897;

/**
 * Consructor for PSO class
 */
PSO::PSO(string neighborhoodTopology, int swarmSize, int numIterations,
	 string function, int numDimensions) {
  this->swarm_size = swarmSize;
  this->num_iterations = numIterations;
  this->num_dimensions = numDimensions;
  this->g_best_value = BIG_DOUBLE;

  /** @TODO: we should make these user definable variables */
  this->phi1 = 2.05;
  this->phi2 = 2.05;
  this->constriction_factor = 0.7298;
  //HEY
  this->k_neighbors = 5;

  // initalize global best position at an arbitrary far away point
  vector <double> gBestPos;
  for(int i = 0; i < num_dimensions; i++){
    gBestPos.push_back(BIG_DOUBLE);
  }
  this->g_best_position = gBestPos;

  // set topology
  if(neighborhoodTopology == "gl") {
    this->neighborhood_topo = GLOBAL;
  }
  else if(neighborhoodTopology == "vn") {
    this->neighborhood_topo = VON_NEUMANN;
  }
  else if(neighborhoodTopology == "ri") {
    this->neighborhood_topo = RING;
  }
  else if(neighborhoodTopology == "ra") {
    this->neighborhood_topo = RANDOM;
  }
  else {
    cout << "Error: Invalid Topolgy Parameter" << endl;
  }

  // set search function initialization bounds
  if(function == "rok") {
    this->min_position = 15;
    this->max_position = 30;
    this->min_velocity = -2;
    this->max_velocity = 2;
    this->function_to_optimize = ROSENBROCK;
  } else if(function == "ack") {
    this->min_position = 16;
    this->max_position = 32;
    this->min_velocity = -2;
    this->max_velocity = 4;
    this->function_to_optimize = ACKLEY;
  } else if (function == "ras" ){
    this->min_position = 2.56;
    this->max_position = 5.12;
    this->min_velocity = -2;
    this->max_velocity = 4;
    this->function_to_optimize = RASTRIGIN;
  } else {
    cout << "Error: Invalid Search Function Parameter" << endl;
  }
  swarm.clear();
  srand(clock());
  initialize_swarm();
}

/**
 * Runs the PSO algorithm in the mode specified by the class member
 * variables (eg, ring topology + Ackely function, etc.)
 */
void PSO::runPSO() {
  double start_time = clock();
  int print_frequency = 30;

  for(int i = 0; i < num_iterations; i++) {
    int print_interval = num_iterations/print_frequency;
    if(i%print_interval == 0) {
      cout << "Iteration " << i << " value: "
	   << g_best_value << "\t";

      cout << "Position " << i << ": ";
      for(int j = 0; j < num_dimensions; j++) {
	cout << g_best_position[j] << " ";
      }
      cout << endl;
    }

    iterate();
  }
  double end_time = clock();
  cout << "Runtime: " << end_time - start_time << endl;
  cout << "Best function value: " << g_best_value << endl;
  cout << "Best position: ";
  for(int j = 0; j < num_dimensions; j++) {
    cout << g_best_position[j] << " ";
  }
  cout << endl;

}

/**
 * Sets the initial positions + velocities of every particle, and sets
 * each particles neighborhoods
 */
void PSO::initialize_swarm() {
  for(int i = 0; i < swarm_size; i++) {
    Particle p;

    // initialize randomly in range
    for(int n = 0; n < num_dimensions; ++ n) {
      p.position.push_back(rand_in_range(min_position, max_position));
      p.velocity.push_back(rand_in_range(min_velocity, max_velocity));
    }// every dimension

    p.p_best_value = function_value(p.position);
    p.p_best_position = p.position;
    p.n_best_value = p.p_best_value; // dummy values, properly set
    p.n_best_position = p.position;  // after this for loop

    // update gbest if necessary
    if(p.p_best_value < g_best_value) {
      g_best_position = p.position;
    }

    swarm.push_back(p);
  }// every particle

  // build topology
  create_neighborhoods();
  evaluate_neighborhoods();
}


/**
 * returns a random double between min and max
 */
double PSO::rand_in_range(double min, double max) {
  return double(rand()/double(RAND_MAX)) * (max - min) + min;
}

/**
 * Calls the appropriate neighborhood creation function, which sets
 * the neighborhood vectors for each particle. DOESN'T evaluate the
 * position/value of neighborhood best
 */
void PSO::create_neighborhoods() {
  switch(neighborhood_topo) {
  case GLOBAL:
    break; //special case, neighborhood logic is handled directly in
	   //class functions per iteration
  case RING:
    create_ring();
    break;
  case VON_NEUMANN:
    create_von_neumann();
    break;
  case RANDOM:
    // If there are fewer particles in the swarm than the neighborhood
    // size, random neighborhood topology becomes equivalent to global
    // topology
    if(k_neighbors < swarm_size) {
      create_random();
    }
    else { // if the swarm is too small to fill a single neigborhood
      neighborhood_topo = GLOBAL;
    }
    break;
  }
}

/**
 * Creates random topology -- this function needs to be called with a
 * probability per iteration, as the random topologies must be
 * recreated to transfer information among the swarm.
 */
void PSO::create_random () {
  for(int i = 0; i < swarm_size; i++) {
    //reset the neighborhood list and best value -- important for re-creation
    swarm[i].neighborhood_indices.clear();
    swarm[i].n_best_value = BIG_DOUBLE;

    // each particle is in its own neighborhood
    swarm[i].neighborhood_indices.push_back(i);

    // k_neighbors is a member variable
    for(int j = 0; j < k_neighbors - 1; j++) {
      int random_index = i;

      // keeps on generating random values in the range from 0 to
      // (swarm_size - 1) until it finds a value that is NOT in the
      // neighborhood array. This guarentees that each particle will
      // have K unique neigbhors indecies in its neighborhood vector

      // find() will search neighborhood vector for the value j. If j
      // is NOT in the vector, returns an iterator afte the end of the
      // array
      while(find(swarm[i].neighborhood_indices.begin(),
		 swarm[i].neighborhood_indices.end(), random_index) !=
	    swarm[i].neighborhood_indices.end()) {
	random_index = rand() % swarm_size;
      }
      swarm[i].neighborhood_indices.push_back(j);
    } // for every neighbor
  } // for every particle
}

/*
 * Particles' neighborhood are the particles directly next and
 * previous in the swarm array
 */
void PSO::create_ring() {
  for(int i = 0; i < swarm_size; i++) {
    // each particle is in its own neighborhood
    swarm[i].neighborhood_indices.push_back(i);
    swarm[i].neighborhood_indices.push_back((i+1) % swarm_size);
    swarm[i].neighborhood_indices.push_back((i-1) % swarm_size);
  }
}


/*
 * Particles' neighborhood are the particles directly next and
 * previous in the swarm array, and also "above" and "below". In order
 * to make this work for any sized array, we defined "above" and
 * "below" as sqrt(swarm size) indecies ahead and behind.
 */
void PSO::create_von_neumann() {
  int width = floor(sqrt(swarm_size));

  // imagine rectangle with width "width", wrap around
  for(int i = 0; i < swarm_size; i++) {
    // each particle is in its own neighborhood
    swarm[i].neighborhood_indices.push_back(i);
    swarm[i].neighborhood_indices.push_back((i+1) % swarm_size);
    swarm[i].neighborhood_indices.push_back(abs((i-1) % swarm_size));
    swarm[i].neighborhood_indices.push_back((i + width) % swarm_size);
    swarm[i].neighborhood_indices.push_back(abs((i - width) % swarm_size));
  }
}

/**
 * Traverses each particle's neighborhood list and finds its best know
 * position.
 *
 * Two cases --
 * NON-GLOBAL: Traverse each particle's neighborhood array and record
 * the best position found.
 *
 * GLOBAL: Grab the global best value from the PSO member variable
 */
void PSO::evaluate_neighborhoods() {
  switch(neighborhood_topo) {
  case GLOBAL:
    for(int i = 0; i < swarm_size; i++) {
      swarm[i].n_best_value = g_best_value;
      swarm[i].n_best_position = g_best_position;
    }
    break;

  default: //needs fixing
    for(int i = 0; i < swarm_size; i++) {
      for(unsigned int j = 0; j < swarm[i].neighborhood_indices.size(); j++) {
	if(swarm[j].p_best_value < swarm[i].n_best_value) { // @Note: I changed [i].p_best to [i].n_bst
	  swarm[i].n_best_value = swarm[j].p_best_value;
	  swarm[i].n_best_position = swarm[j].p_best_position;
	}//end if
      }//end inner for
    }//end outer for
    break;
  }//end case
}//end func

void PSO::update_velocities() {
  for(int i = 0; i < swarm_size; i++) {
    // HOT-SPOT FOR THE BUGS
    vector<double> p_best_difference =
      vector_subtraction(swarm[i].p_best_position, swarm[i].position);
    vector<double> n_best_difference =
      vector_subtraction(swarm[i].n_best_position, swarm[i].position);

    for(int j = 0; j < num_dimensions; j++) {
      // @TODO: There appears to be a seg-fault bug right here
      // Apparently, n_best_position is not set? breakpoint at those lines.
      swarm[i].velocity[j] =
	constriction_factor *
	(swarm[i].velocity[j] + rand_in_range(0,phi1) * p_best_difference[j]
	 + rand_in_range(0,phi2) * n_best_difference[j]);
    }
  }
}

void PSO::update_positions() {
  for(int i = 0; i < swarm_size; i++) {
    for(int j = 0; j < num_dimensions; j++) {
      swarm[i].position[j] = swarm[i].position[j] + swarm[i].velocity[j];
    }
  }
}

void PSO::evaluate_swarm() {
  for(int i = 0; i < swarm_size; i++) {
    double p_value = function_value(swarm[i].position);
    if(p_value < swarm[i].p_best_value) {
      swarm[i].p_best_value = p_value;
      swarm[i].p_best_position = swarm[i].position;
      if(p_value < g_best_value) {
	g_best_value = p_value;
	g_best_position = swarm[i].position;
      }
    }
  }
}

double PSO::function_value(vector<double> position) {
  switch(function_to_optimize) {
  case ACKLEY:
    return ackley_function(position);
  case ROSENBROCK:
    return rosenbrock_function(position);
  case RASTRIGIN:
    return rastrigin_function(position);
  }
  // whats the right way to do this?
  cout << "ERROR ERROR ERROR\n" << endl;
  return BIG_DOUBLE;
}

double PSO::rosenbrock_function(vector<double> position) {
  double value = 0;
  for(int i = 0; i < num_dimensions-1; i++) {
    value += 100 * pow(-pow(position[i], 2) + position[i+1], 2) + pow(position[i - 1] - 1, 2);
  }
  return value;
}

double PSO::ackley_function(vector<double> position) {
  double sum_one = 0;
  double sum_two = 0;

  for(int i = 0; i < num_dimensions; i++) {
    sum_one += pow(position[i], 2);
    //    sum_two += (position[i], 2) * cos(2 * M_PI * position[i]); //math.h const
    sum_two += cos(2 * M_PI * position[i]);
  }

  return -20 * exp(-0.2 * sqrt(sum_one / num_dimensions)) -
    exp(sum_two / num_dimensions) + 20 + exp(1);
}

double PSO::rastrigin_function(vector<double> position) {
  double value = 0;
  for(int i = 0; i < num_dimensions; i++) {
    value += pow(position[i], 2) - 10 * cos(2 * PI * position[i]) + 10;
  }
  return value;
}

// element-wise vector subtraction
vector<double> PSO::vector_subtraction(vector<double> vector1, vector<double> vector2) {
  vector<double> vector_difference;
  for(unsigned int i = 0; i < vector1.size(); i++) {
    vector_difference.push_back(vector1[i] - vector2[i]);
  }
  return vector_difference;
}


void PSO::iterate() {
  update_velocities();
  update_positions();
  evaluate_swarm();

  if(neighborhood_topo == RANDOM) {
    // @TODO: Hardcoded mutation rate - change to class variable?
    double random_percent = (double) rand() / (RAND_MAX);
    if(random_percent < 0.2) create_random();
  }
  evaluate_neighborhoods();
}
