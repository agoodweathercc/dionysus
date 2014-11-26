template<class F, typename I, class C, template<class Self> class... V>
void
dionysus::ReducedMatrix<F,I,C,V...>::
resize(size_t s)
{
    reduced_.resize(s);
    pairs_.resize(s, unpaired());
}

template<class F, typename I, class C, template<class Self> class... V>
template<class ChainRange>
typename dionysus::ReducedMatrix<F,I,C,V...>::Index
dionysus::ReducedMatrix<F,I,C,V...>::
add(const ChainRange& chain)
{
    // TODO: skip the computation entirely if we already know this is positive (in case of the clearing optimization)
    Index i = pairs_.size();
    pairs_.emplace_back(unpaired());
    reduced_.emplace_back();

    return set(i, chain);
}

template<class F, typename I, class C, template<class Self> class... V>
template<class ChainRange>
typename dionysus::ReducedMatrix<F,I,C,V...>::Index
dionysus::ReducedMatrix<F,I,C,V...>::
set(Index i, const ChainRange& chain)
{
    auto entry_cmp = [this](const Entry& e1, const Entry& e2) { return this->cmp_(e1.index(), e2.index()); };

    Chain   c(std::begin(chain), std::end(chain));
    std::sort(c.begin(), c.end(), entry_cmp);
    visitors_chain_initialized(c);
    reduced_[i] = std::move(c);

    return reduce(i);
}

template<class F, typename I, class C, template<class Self> class... V>
typename dionysus::ReducedMatrix<F,I,C,V...>::Index
dionysus::ReducedMatrix<F,I,C,V...>::
reduce(Index i)
{
    auto entry_cmp = [this](const Entry& e1, const Entry& e2) { return this->cmp_(e1.index(), e2.index()); };

    Chain& c    = column(i);
    Index  pair = Reduction<Index>::reduce(c, reduced_, pairs_, field_,
                                          [this](FieldElement m, Index cl)
                                          { this->visitors_addto<>(m, cl); },
                                          entry_cmp);

    if (pair != unpaired())
        pairs_[pair] = i;

    pairs_[i]   = pair;
    visitors_reduction_finished<>();

    return pair;
}