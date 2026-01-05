#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/vec3.hpp>

// Kepler Element With Rate
struct KEWR {
	double value;
	double rate;
	double AtTime(double time) const;
};

struct KElements {
	KEWR a_wr, e_wr, I_wr, L_wr, lp_wr, ln_wr;
	glm::dvec3 AtTime(double time) const;
private:
	double GetEccentricAnomaly(double eccentricity, double meanAnomaly) const;
};

class Planet {
public:
	Planet(std::string_view name, const KElements& elements);
	const std::string& GetName() const;
	glm::dvec3 AtTime(double time) const;
private:
	std::string _name;
	KElements _elements;
};

class SolarSystem {
public:
	SolarSystem();
	Planet* mercury;
	Planet* venus;
	Planet* earth;
	Planet* mars;
	Planet* jupiter;
	Planet* saturn;
	Planet* uranus;
	Planet* neptune;
private:
	std::vector<std::unique_ptr<Planet>> _planets;

};