#include "core/common.h"
#include "server/loop-notifier.h"
#include "zen-remote/server/peer-manager.h"

namespace zen::remote::server {

class Peer;

class PeerManager final : public IPeerManager, public ILoopNotifierDelegate {
  enum Signal : uint8_t {
    kPortForwardedProbeSuccess = 0,
    kPortForwardedProbeFailure,
  };

 public:
  DISABLE_MOVE_AND_COPY(PeerManager);
  PeerManager() = delete;
  PeerManager(std::unique_ptr<ILoop> loop);
  ~PeerManager();

  bool Init();

  std::shared_ptr<IPeer> Get(uint64_t peer_id) override;

  void Notify(uint8_t signal) override;

 private:
  bool ListenUdpBroadcast();
  void AcceptUdpBroadcast(int fd, uint32_t mask);
  void StartPortForwardedPeerProber();
  void StopPortForwardedPeerProber();
  void PortForwardedPeerProbe(bool success);

  std::list<std::shared_ptr<Peer>> peers_;
  std::shared_ptr<ILoop> loop_;
  boost::asio::io_context io_context_;
  boost::asio::ip::udp::socket udp_socket_;
  std::unique_ptr<FdSource> udp_socket_source_;

  LoopNotifier notifier_;
  std::thread port_forwarded_peer_prober_;
  struct {
    bool enable = false;
    std::mutex mtx;
    std::condition_variable cond;
  } prober_;
};

}  // namespace zen::remote::server
