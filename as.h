#ifndef AS_H
#define AS_H


class AS
{
public:
    AS();
    AS(const AS& a):as_id{a.as_id}{}
    int id();
private:
    int as_id;
};

#endif // AS_H
