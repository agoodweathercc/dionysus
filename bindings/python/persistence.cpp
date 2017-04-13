#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include <dionysus/row-reduction.h>
#include <dionysus/ordinary-persistence.h>
#include <dionysus/standard-reduction.h>

#include "field.h"
#include "filtration.h"
#include "persistence.h"
#include "diagram.h"

PyReducedMatrix
homology_persistence(const PyFiltration& filtration, PyZpField::Element prime, std::string method)
{
    PyZpField field(prime);

    if (method == "row")
    {
        using Reduction = dionysus::RowReduction<PyZpField>;
        Reduction   reduce(field);
        reduce(filtration);
        return reduce.persistence();
    } else if (method == "column")
    {
        using Persistence = dionysus::OrdinaryPersistence<PyZpField>;
        using Reduction   = dionysus::StandardReduction<Persistence>;
        Persistence persistence(field);
        Reduction   reduce(persistence);
        reduce(filtration);
        return std::move(reduce.persistence());
    } else if (method == "column_no_negative")
    {
        using Persistence = dionysus::OrdinaryPersistenceNoNegative<PyZpField>;
        using Reduction   = dionysus::StandardReduction<Persistence>;
        Persistence persistence(field);
        Reduction   reduce(persistence);
        reduce(filtration);
        return std::move(reduce.persistence());
    } else
        throw std::runtime_error("Unknown method: " + method);
}

bool chain_eq(const PyReducedMatrix::Chain& c1, const PyReducedMatrix::Chain& c2)
{
    if (c1.size() != c2.size())
        return false;

    for (size_t i = 0; i < c1.size(); ++i)
    {
        if (c1[i].index() != c2[i].index() || c1[i].element() != c2[i].element())
            return false;
    }
    return true;
}

bool chain_ne(const PyReducedMatrix::Chain& c1, const PyReducedMatrix::Chain& c2)
{
    return !chain_eq(c1, c2);
}

std::vector<PyDiagram>
py_init_diagrams(const PyReducedMatrix& m, const PyFiltration& f)
{
    return init_diagrams(m, f,
                         [](const PySimplex& s)                     { return s.data(); },       // value
                         [](PyReducedMatrix::Index i) -> size_t     { return i; });             // data
}

PYBIND11_MAKE_OPAQUE(PyReducedMatrix::Chain);      // we want to provide our own binding for Chain

void init_persistence(py::module& m)
{
    using namespace pybind11::literals;
    m.def("homology_persistence",   &homology_persistence, "filtration"_a, py::arg("prime") = 2, py::arg("method") = "row",
          "compute homology persistence of the filtration (pair simplices); method is one of `row`, `column`, or `column_no_negative`");

    m.def("init_diagrams",      &py_init_diagrams,  "m"_a, "f"_a,  "initialize diagrams from reduced matrix and filtration");

    py::class_<PyReducedMatrix>(m, "ReducedMatrix", "matrix, where each column has a lowest non-zero entry in a unique row; supports iteration and indexing")
        .def(py::init<PyZpField>())
        .def("__len__",     &PyReducedMatrix::size,         "size of the matrix")
        .def("__getitem__", &PyReducedMatrix::operator[],   "access the column at a given index")
        .def("pair",        &PyReducedMatrix::pair,         "pair of the given index")
        .def_property_readonly("unpaired",      [](const PyReducedMatrix&) { return PyReducedMatrix::unpaired(); },
                               "index representing lack of pair")
        .def("__iter__",    [](const PyReducedMatrix& rm)   { return py::make_iterator(rm.columns().begin(), rm.columns().end()); },
                                py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */,
                                "iterate over the columns of the matrix")
        .def("__repr__",    [](const PyReducedMatrix& rm)
                            { std::ostringstream oss; oss << "Reduced matrix with " << rm.size() << " columns"; return oss.str(); })
    ;

    py::class_<PyReducedMatrix::Chain>(m, "Chain", "chain of indices (formal sum with coefficients in Zp)")
        .def("__len__",     &PyReducedMatrix::Chain::size,                                  "size of the chain")
        .def("__getitem__", [](const PyReducedMatrix::Chain& c, size_t i) { return c[i]; }, "access the entry at a given index")
        .def("__iter__",    [](const PyReducedMatrix::Chain& c) { return py::make_iterator(c.begin(), c.end()); },
                                py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */,
                                "iterate over the entries of the chain")
        .def("__eq__",      &chain_eq, "equality comparison")
        .def("__ne__",      &chain_ne, "nonequal comparison")
        .def("__repr__",    [](const PyReducedMatrix::Chain& c)
                            {
                                std::ostringstream oss;
                                auto it = c.begin();
                                if (it == c.end())
                                    return oss.str();
                                oss << it->e << '*' << it->i;
                                while (++it != c.end())
                                    oss << " + " << it->e << '*' << it->i;
                                return oss.str();
                            })
    ;

    py::class_<PyReducedMatrix::Entry>(m, "ChainEntry", "(coefficient, index) entry in a chain)")
        .def_property_readonly("element",   [](const PyReducedMatrix::Entry& x) { return x.element(); },
                                            "coefficient of the chain element")
        .def_readonly("index",              &PyReducedMatrix::Entry::i,   "index of the chain element")
        .def("__repr__",                    [](const PyReducedMatrix::Entry& e)
                                            { std::ostringstream oss; oss << e.e << '*' << e.i; return oss.str(); })
    ;
}
