#ifndef CRYPTO_MSG_PARSER_H_
#define CRYPTO_MSG_PARSER_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "crypto_market_type.h"
#include "crypto_msg_type.h"

enum TradeSide {
  /** Buyer is taker */
  kBuy,
  /** Seller is taker */
  kSell,
};

/** Base struct of all parsed message. */
struct ParsedMessage {
  /** The exchange name, unique for each exchage */
  std::string exchange;
  /** Market type */
  MarketType market_type;
  /** Message type */
  MessageType msg_type;
  /** Exchange-specific trading symbol or id, recognized by RESTful API */
  std::string symbol;
  /** Unified pair, base/quote, e.g., BTC/USDT */
  std::string pair;
  /** Unix timestamp in milliseconds */
  uint64_t timestamp;
  /** the original message */
  std::string json;
};

/** Realtime trade message. */
struct TradeMsg : public ParsedMessage {
  /** price */
  double price;
  /** Number of base coins */
  double quantity_base;
  /** Number of quote coins(mostly USDT) */
  double quantity_quote;
  /** Number of contracts, always None for Spot */
  std::optional<double> quantity_contract;
  /** Which side is taker */
  TradeSide side;
  /** Trade ID */
  std::string trade_id;
};

/** An order in the orderbook asks or bids array. */
struct Order {
  /** price */
  double price;
  /** Number of base coins, 0 means the price level can be removed */
  double quantity_base;
  /** Number of quote coins(mostly USDT) */
  double quantity_quote;
  /** Number of contracts, always None for Spot */
  std::optional<double> quantity_contract;
};

/** Level2 orderbook message. */
struct OrderBookMsg : public ParsedMessage {
  /** sorted in ascending order by price if snapshot=true,otherwise not sorted
   */
  std::vector<Order> asks;
  /** sorted in descending order by price if snapshot=true, otherwise not sorted
   */
  std::vector<Order> bids;
  /** true means snapshot, false means updates */
  bool snapshot;
  /** The sequence ID for this update (not all exchanges provide this
   * information) */
  std::optional<uint64_t> seq_id;
  /** The sequence ID for the previous update (not all exchanges provide this
   * information) */
  std::optional<uint64_t> prev_seq_id;
};

/** Funding rate message. */
struct FundingRateMsg : public ParsedMessage {
  /** Funding rate, which is calculated on data between [funding_time-16h,
   * funding_time-8h] */
  double funding_rate;
  // Funding time, the moment when funding rate is used
  int64_t funding_time;
  /** Estimated funding rate between [funding_time-h, funding_time], it will be
   * static after funding_time */
  std::optional<uint64_t> estimated_rate;
};

/**
 * Extract the symbol from the message.
 */
std::string extract_symbol(std::string_view exchange, MarketType market_type,
                           std::string_view msg);

/**
 * Infer the message type from the message.
 */
MessageType get_msg_type(std::string_view exchange, std::string_view msg);

/**
 * Parse a raw trade message into a Vec<TradeMsg> and then convert to a JSON
 * string.
 */
std::vector<TradeMsg> parse_trade(std::string_view exchange,
                                  MarketType market_type, std::string_view msg);

/**
 * Parse a raw level2 orderbook message into a Vec<OrderBookMsg> and then
 * convert to a JSON string.
 */
std::vector<OrderBookMsg> parse_l2(std::string_view exchange,
                                   MarketType market_type, std::string_view msg,
                                   int64_t timestamp);

/**
 * Parse a raw funding rate message into a Vec<FundingRateMsg> and then convert
 * to a JSON string.
 */
std::vector<FundingRateMsg> parse_funding_rate(std::string_view exchange,
                                               MarketType market_type,
                                               std::string_view msg);

#endif /* CRYPTO_MSG_PARSER_H_ */
