#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/vec3.hpp>

class SolarBody;

class SolarBodyDriver {
public:
	virtual glm::dvec3 GetPositionAtTime(double time) const = 0;
};

class KeplerOrbit : public SolarBodyDriver {
public:
	KeplerOrbit(SolarBody* parentBody, double a, double e, double w, double M, double I, double ln);
	double a, e, w, M, I, ln;
	SolarBody* parentBody;
	glm::dvec3 GetPositionAtTime(double time) const override;
};

struct VaryingElement {
	double value;
	double rate;
	double GetValueAtTime(double time) const;
};

class VaryingKeplerOrbit : public SolarBodyDriver {
public:
	VaryingKeplerOrbit(SolarBody* parentBody, VaryingElement a_wr, VaryingElement e_wr, VaryingElement I_wr, VaryingElement L_wr, VaryingElement lp_wr, VaryingElement ln_wr);
	VaryingElement a_wr, e_wr, I_wr, L_wr, lp_wr, ln_wr;
	SolarBody* parentBody;
	glm::dvec3 GetPositionAtTime(double time) const override;
};

class SolarBody {
public:
	SolarBody(std::string_view name, double radius, SolarBodyDriver* driver = nullptr);
	const std::string& GetName() const;
	double GetRadius() const;
	glm::dvec3 GetPositionAtTime(double time) const;
private:
	std::unique_ptr<SolarBodyDriver> _driver;
	std::string _name;
	double _radius;
};

class SolarSystem {
public:
	SolarSystem();
	SolarBody* sun;
	SolarBody* mercury;
	SolarBody* venus;
	SolarBody* earth;
	SolarBody* earthLuna;
	SolarBody* mars;
	SolarBody* jupiter;
	SolarBody* saturn;
	SolarBody* uranus;
	SolarBody* neptune;
	std::vector<std::unique_ptr<SolarBody>> bodies;
	SolarBody* GetBody(std::string_view bodyName);
private:
	SolarBody* AddBody(SolarBody* body);
};