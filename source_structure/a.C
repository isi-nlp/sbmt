        constituent_ordered_list::const_iterator it_const_list = m_fconstituent_list.find(result_span); 

        // if the decoder span does not match any foreign span in the foreign constituent list,
        // we iterate over the sorted list, and find all the overlapping spans.
        // we create the following features:
        // - the target_root_cross
        // - the source_label_cross
        // - the source-label_target-root_cross
        if(it_const_list == m_fconstituent_list.end()){
                    score_t sc = score_t(1.0, as_neglog10());
                    ostringsteam ost;
                    ost<<"other_cross";
                    boost::uint32_t fid=g.feature_names().get_index(ost.str());
                    *accum = make_pair(fid, sc);
                    ++accum;
                    inside*= sc ^ grammar.get_weights()[fid];
        } 
        // the decoder span matches a foreign parse span in the constituent list.
        else {

            BOOST_FOREACH(constituent_list::data_type::const_iterator it_data, *it_const_list){
                // if the nt is in the list that we are concerned.
                if(m_concerned_nts.find(*it_data) == m_concerned_nts){ // \\TODO
                    // feature name nt_match
                    score_t sc = score_t(1.0, as_neglog10());
                    ostringsteam ost;
                    ost<<*it_data<<"_match";
                    boost::uint32_t fid=g.feature_names().get_index(ost.str());
                    *accum = make_pair(fid, sc);
                    ++accum;
                    inside*= sc ^ grammar.get_weights()[fid];

                } else {
                    // feature name nt_cross
                    score_t sc = score_t(1.0, as_neglog10());
                    ostringsteam ost;
                    ost<<*it_data<<"_cross";
                    boost::uint32_t fid=g.feature_names().get_index(ost.str());
                    *accum = make_pair(fid, sc);
                    ++accum;
                    inside*= sc ^ grammar.get_weights()[fid];
                }
            }
        }

