#include"Boss/Mod/ActiveProber.hpp"
#include"Boss/Mod/AutoDisconnector.hpp"
#include"Boss/Mod/BlockTracker.hpp"
#include"Boss/Mod/BoltzSwapper/Main.hpp"
#include"Boss/Mod/ChannelCandidateInvestigator/Main.hpp"
#include"Boss/Mod/ChannelCandidateMatchmaker.hpp"
#include"Boss/Mod/ChannelCandidatePreinvestigator.hpp"
#include"Boss/Mod/ChannelCreationDecider.hpp"
#include"Boss/Mod/ChannelCreator/Main.hpp"
#include"Boss/Mod/ChannelFeeManager.hpp"
#include"Boss/Mod/ChannelFeeSetter.hpp"
#include"Boss/Mod/ChannelFinderByDistance.hpp"
#include"Boss/Mod/ChannelFinderByListpays.hpp"
#include"Boss/Mod/ChannelFinderByPopularity.hpp"
#include"Boss/Mod/ChannelFundsComputer.hpp"
#include"Boss/Mod/ConnectFinderByDns.hpp"
#include"Boss/Mod/ConnectFinderByHardcode.hpp"
#include"Boss/Mod/Connector.hpp"
#include"Boss/Mod/CommandReceiver.hpp"
#include"Boss/Mod/ForwardFeeMonitor.hpp"
#include"Boss/Mod/HtlcAcceptor.hpp"
#include"Boss/Mod/InitialConnect.hpp"
#include"Boss/Mod/Initiator.hpp"
#include"Boss/Mod/InternetConnectionMonitor.hpp"
#include"Boss/Mod/InvoicePayer.hpp"
#include"Boss/Mod/JsonOutputter.hpp"
#include"Boss/Mod/ListfundsAnnouncer.hpp"
#include"Boss/Mod/ListpaysHandler.hpp"
#include"Boss/Mod/ListpeersAnalyzer.hpp"
#include"Boss/Mod/ListpeersAnnouncer.hpp"
#include"Boss/Mod/Manifester.hpp"
#include"Boss/Mod/NeedsConnectSolicitor.hpp"
#include"Boss/Mod/NeedsOnchainFundsSwapper.hpp"
#include"Boss/Mod/NewaddrHandler.hpp"
#include"Boss/Mod/NodeBalanceSwapper.hpp"
#include"Boss/Mod/OnchainFeeMonitor.hpp"
#include"Boss/Mod/OnchainFundsAnnouncer.hpp"
#include"Boss/Mod/PeerCompetitorFeeMonitor/Main.hpp"
#include"Boss/Mod/PeerMetrician.hpp"
#include"Boss/Mod/PeerStatistician.hpp"
#include"Boss/Mod/Reconnector.hpp"
#include"Boss/Mod/RegularActiveProbe.hpp"
#include"Boss/Mod/SendpayResultMonitor.hpp"
#include"Boss/Mod/StatusCommand.hpp"
#include"Boss/Mod/SwapManager.hpp"
#include"Boss/Mod/Timers.hpp"
#include"Boss/Mod/Waiter.hpp"
#include"Boss/Mod/all.hpp"
#include<vector>

namespace {

class All {
private:
	std::vector<std::shared_ptr<void>> modules;

public:
	template<typename M, typename... As>
	std::shared_ptr<M> install(As&&... as) {
		auto ptr = std::make_shared<M>(as...);
		modules.push_back(std::shared_ptr<void>(ptr));
		return ptr;
	}
};

}

namespace Boss { namespace Mod {

std::shared_ptr<void> all( std::ostream& cout
			 , S::Bus& bus
			 , Ev::ThreadPool& threadpool
			 , std::function< Net::Fd( std::string const&
						 , std::string const&
						 )
					> open_rpc_socket
			 ) {
	auto all = std::make_shared<All>();

	/* Basic.  */
	auto waiter = all->install<Waiter>(bus);
	auto imon = all->install<InternetConnectionMonitor>( bus
							   , threadpool
							   , *waiter
							   );
	all->install<JsonOutputter>(cout, bus);
	all->install<CommandReceiver>(bus);

	/* Startup.  */
	all->install<Manifester>(bus);
	all->install<Initiator>(bus, threadpool, std::move(open_rpc_socket));

	/* Regular timers.  */
	all->install<BlockTracker>(bus);
	all->install<Timers>(bus, *waiter);

	/* Connection wrangling.  */
	all->install<InitialConnect>(bus);
	all->install<Connector>(bus);
	all->install<NeedsConnectSolicitor>(bus);
	all->install<ConnectFinderByDns>(bus);
	all->install<ConnectFinderByHardcode>(bus);
	all->install<Reconnector>(bus);
	all->install<AutoDisconnector>(bus);

	/* Status monitors.  */
	all->install<ChannelFundsComputer>(bus);
	all->install<ListpeersAnnouncer>(bus);
	all->install<ListpeersAnalyzer>(bus);
	all->install<OnchainFeeMonitor>(bus, *waiter);
	all->install<ListfundsAnnouncer>(bus);
	all->install<OnchainFundsAnnouncer>(bus);

	/* Channel creation wrangling.  */
	all->install<ChannelFinderByDistance>(bus, *waiter);
	all->install<ChannelFinderByListpays>(bus);
	all->install<ChannelFinderByPopularity>(bus, *waiter);
	all->install<ChannelCandidatePreinvestigator>(bus);
	auto investigator = all->install< ChannelCandidateInvestigator::Main
					>(bus, *imon);
	all->install<ChannelCreationDecider>(bus);
	all->install<ChannelCreator::Main>(bus, *waiter, *investigator);
	all->install<ChannelCandidateMatchmaker>(bus);

	/* Status.  */
	all->install<StatusCommand>(bus);

	/* Offchain-to-onchain swap.  */
	all->install<NewaddrHandler>(bus);
	all->install<BoltzSwapper::Main>(bus, threadpool);
	all->install<SwapManager>(bus);
	all->install<NeedsOnchainFundsSwapper>(bus);
	all->install<NodeBalanceSwapper>(bus);

	/* Invoice wrangling.  */
	all->install<InvoicePayer>(bus);
	all->install<ListpaysHandler>(bus);

	/* Channel fees.  */
	all->install<PeerCompetitorFeeMonitor::Main>(bus);
	all->install<ChannelFeeSetter>(bus);
	all->install<ChannelFeeManager>(bus);

	/* HTLC manipulation.  */
	all->install<HtlcAcceptor>(bus, *waiter);

	/* Bad peer monitoring.  */
	all->install<SendpayResultMonitor>(bus);
	all->install<ForwardFeeMonitor>(bus);
	all->install<PeerStatistician>(bus);
	all->install<PeerMetrician>(bus);
	all->install<ActiveProber>(bus, *investigator);
	all->install<RegularActiveProbe>(bus);

	return all;
}

}}
