#include "crypto_msg_parser.h"

#include <gtest/gtest.h>

#include <optional>

TEST(TradeMsgTest, ParseTrade) {
  std::cout << "### line 6" << std::endl;
  // NOLINTNEXTLINE
  const char raw_msg[] = R"json(
    {
    "stream":"btcusdt@aggTrade",
      "data":{
          "e":"aggTrade",
          "E":1616202009196,
          "a":389551486,
          "s":"BTCUSDT",
          "p":"58665.00",
          "q":"0.043",
          "f":621622993,
          "l":621622993,
          "T":1616202009188,
          "m":false
      }
    }
  )json";
  std::vector<TradeMsg> messages =
      parse_trade("binance", MarketType::LinearSwap, raw_msg);
  ASSERT_EQ(messages.size(), 1);

  const TradeMsg& trade_msg = messages[0];

  ASSERT_EQ(trade_msg.exchange, "binance");
  ASSERT_EQ(trade_msg.market_type, MarketType::LinearSwap);
  ASSERT_EQ(trade_msg.msg_type, MessageType::Trade);
  ASSERT_EQ(trade_msg.symbol, "BTCUSDT");
  ASSERT_EQ(trade_msg.pair, "BTC/USDT");
  ASSERT_EQ(trade_msg.price, 58665.0);
  ASSERT_EQ(trade_msg.quantity_base, 0.043);
  ASSERT_EQ(trade_msg.quantity_quote, 0.043 * 58665.0);
  ASSERT_EQ(trade_msg.quantity_contract, std::optional<double>(0.043));
  ASSERT_EQ(trade_msg.side, TradeSide::kBuy);
}
