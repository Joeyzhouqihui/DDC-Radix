#include <stdio.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/coroutine/all.hpp>

using Coroutine_t = boost::coroutines::symmetric_coroutine<void>;
using CoroYield = boost::coroutines::symmetric_coroutine<void>::yield_type;
using CoroCall = boost::coroutines::symmetric_coroutine<void>::call_type;

int main()
{

	Coroutine_t::call_type coro_recv(
		[&](Coroutine_t::yield_type& yield) {
		for (int i=0; i<3; i++) {
         std::cout<<i<<std::endl;
         (yield)(coro_send);
      }
	});

   Coroutine_t::call_type coro_send(
		[&](Coroutine_t::yield_type& yield) {
		for (int i=3; i<6; i++) {
         std::cout<<i<<std::endl;
         (yield)(coro_recv);
      }
	});

	coro_send();
	return 0;
}