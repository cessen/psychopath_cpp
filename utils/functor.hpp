#ifndef FUNCTOR_HPP
#define FUNCTOR_HPP

class Functor
{
public:
	virtual ~Functor() {}
	virtual void operator()() = 0;
};

#endif // FUNCTOR_HPP
