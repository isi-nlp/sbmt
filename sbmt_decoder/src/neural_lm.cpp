# include <sbmt/ngram/neural_lm.hpp>
# include <cfloat>

void nplm_model::set_input_vocabulary(const nplm::vocabulary &vocab)
{
    *(this->input_vocab) = vocab;
    start = vocab.lookup_word("<s>");
    null = vocab.lookup_word("<null>");
    end = vocab.lookup_word("</s>");
}

void nplm_model::set_output_vocabulary(const nplm::vocabulary &vocab)
{
    *(this->output_vocab) = vocab;
}

size_t nplm_model::read(const std::string &filename)
{
    std::vector<std::string> input_words;
    std::vector<std::string> output_words;
    m->read(filename, input_words, output_words);
    set_input_vocabulary(nplm::vocabulary(input_words));
    set_output_vocabulary(nplm::vocabulary(output_words));
    m->premultiply();
    return m->ngram_size;
}

nplm_model::nplm_model(std::string const& filename, char md, bool normalize) 
: m(new nplm::model())
, input_vocab(new nplm::vocabulary())
, output_vocab(new nplm::vocabulary())
, map_digits(md)
, normalize(normalize)
, p(this)
, d(this)
{ 
  int n = read(filename);
  dcache.reset(new graehl::dynamic_hash_cache<div,graehl::spin_locking>(d,n-1,normalize? 1000000 : 10));
  cache.reset(new graehl::dynamic_hash_cache<prob,graehl::spin_locking>(p,n,10000000));
}

void nplm_model::set_map_digits(char value) { map_digits = value; }

const nplm::vocabulary& nplm_model::get_input_vocabulary() const { return *(this->input_vocab); }

const nplm::vocabulary& nplm_model::get_output_vocabulary() const { return *(this->output_vocab); }

int nplm_model::lookup_input_word(const std::string& word) const
{
    if (map_digits)
        for (int i=0; i<word.length(); i++)
            if (isdigit(word[i]))
            {
                std::string mapped_word(word);
                for (; i<word.length(); i++) if (isdigit(word[i])) mapped_word[i] = map_digits;
                return input_vocab->lookup_word(mapped_word);
            }
    return input_vocab->lookup_word(word);
}

int nplm_model::lookup_output_word(const std::string& word) const
{
    if (map_digits)
        for (int i=0; i<word.length(); i++)
            if (isdigit(word[i]))
            {
                std::string mapped_word(word);
                for (; i<word.length(); i++)
                    if (isdigit(word[i]))
                        mapped_word[i] = map_digits;
                return output_vocab->lookup_word(mapped_word);
            }
    return output_vocab->lookup_word(word);
}

double nplm_model::lookup_ngram(const boost::uint32_t* ngram_a, int n) const
{
  return (*cache)(ngram_a,n);
}

double nplm_model::lookup_ngram_uncached(const boost::uint32_t *ngram_a, int n) const
{
    if (n != m->ngram_size) {
        std::stringstream sstr;
        sstr << m->ngram_size << " != " << n;
	throw std::runtime_error(sstr.str());
    }
    if (not nlms.get()) nlms.reset(new nplm::propagator(*m,1));
    Eigen::Matrix<int,Eigen::Dynamic,1> ngram(m->ngram_size);
    for (int i=0; i<m->ngram_size; i++)
    {
        if (i-m->ngram_size+n < 0)
        {
            if (ngram_a[0] == start)
                ngram(i) = start;
            else
                ngram(i) = null;
        }
        else
        {
            ngram(i) = ngram_a[i-m->ngram_size+n];
        }
    }
    double log_denom = (*dcache)(ngram_a,n-1);
    double log_prob = lookup_ngram(ngram,*(nlms.get()));
    if (std::isnan(log_prob)) {
        std::cerr << "neural_lm: ";
        BOOST_FOREACH(boost::uint32_t w, std::make_pair(ngram_a,ngram_a+n)) {
            std::cerr << w << ' ';
        }
        std::cerr << " -> " << log_prob << '\n';
    }
    return log_prob - log_denom;
    
}

double nplm_model::lookup_ngram_divisor_uncached(const boost::uint32_t *ngram_a, int n) const
{
    n += 1;
    if (normalize == false) return 0.0;
    else {
        Eigen::Matrix<int,Eigen::Dynamic,1> ngram(m->ngram_size);
        for (int i=0; i<m->ngram_size; i++)
        {
            if (i-m->ngram_size+n < 0)
            {
                if (ngram_a[0] == start)
                    ngram(i) = start;
                else
                    ngram(i) = null;
            }
            else
            {
                ngram(i) = ngram_a[i-m->ngram_size+n];
            }
        }
        nplm::propagator& prop = *nlms;
        prop.fProp(ngram.col(0));
        Eigen::Matrix<double,Eigen::Dynamic,1> scores(m->output_vocab_size);
        prop.output_layer_node.param->fProp(prop.second_hidden_activation_node.fProp_matrix, scores);
        double logz = nplm::logsum(scores.col(0));
        return logz;
    }
}

double nplm_model::lookup_ngram(Eigen::Matrix<int,Eigen::Dynamic,1>& ngram, nplm::propagator& prop) const
{
    prop.fProp(ngram.col(0));
    int output = ngram(m->ngram_size-1, 0);
    double log_prob;
    log_prob =  prop.output_layer_node.param->fProp(prop.second_hidden_activation_node.fProp_matrix, output, 0);
    return log_prob;
}
