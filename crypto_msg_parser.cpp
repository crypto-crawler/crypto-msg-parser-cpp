#include "crypto_msg_parser.h"

#include <iostream>
#include <nlohmann/json.hpp>

#include "crypto_msg_parser_ffi.h"

// map TradeSide values to JSON as strings
NLOHMANN_JSON_SERIALIZE_ENUM(TradeSide, {
                                            {kBuy, "buy"},
                                            {kSell, "sell"},
                                        })

NLOHMANN_JSON_SERIALIZE_ENUM(MarketType,
                             {
                                 {Unknown, "unknown"},
                                 {Spot, "spot"},
                                 {LinearFuture, "linear_future"},
                                 {InverseFuture, "inverse_future"},
                                 {LinearSwap, "linear_swap"},
                                 {InverseSwap, "inverse_swap"},
                                 {AmericanOption, "american_option"},
                                 {EuropeanOption, "european_option"},
                                 {Move, "move"},
                                 {BVOL, "bvol"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(MessageType, {
                                              {Other, "other"},
                                              {Trade, "trade"},
                                              {L2Event, "l2_event"},
                                              {L2Snapshot, "l2_snapshot"},
                                              {L2TopK, "l2_topk"},
                                              {L3Event, "l3_event"},
                                              {L3Snapshot, "l3_snapshot"},
                                              {BBO, "bbo"},
                                              {Ticker, "ticker"},
                                              {Candlestick, "candlestick"},
                                              {FundingRate, "funding_rate"},
                                              {OpenInterest, "open_interest"},
                                          })

static void to_json_shared(nlohmann::json& j, const Message& msg) {
  j["exchange"] = msg.exchange;
  j["market_type"] = msg.market_type;
  j["msg_type"] = msg.msg_type;
  j["symbol"] = msg.symbol;
  j["pair"] = msg.pair;
  j["timestamp"] = msg.timestamp;
  j["json"] = msg.json;
}

static void to_json(nlohmann::json& j, const TradeMsg& msg) {
  to_json_shared(j, msg);
  j["price"] = msg.price;
  j["quantity_base"] = msg.quantity_base;
  j["quantity_quote"] = msg.quantity_quote;
  if (msg.quantity_contract != std::nullopt) {
    j["quantity_contract"] = msg.quantity_contract.value();
  }
  j["side"] = msg.side;
  j["trade_id"] = msg.trade_id;
}

static void to_json(nlohmann::json& j, const Order& order) {
  j["price"] = order.price;
  j["quantity_base"] = order.quantity_base;
  j["quantity_quote"] = order.quantity_quote;
  if (order.quantity_contract != std::nullopt) {
    j["quantity_contract"] = order.quantity_contract.value();
  }
}

static void to_json(nlohmann::json& j, const OrderBookMsg& msg) {
  to_json_shared(j, msg);
  j["asks"] = msg.asks;
  j["bids"] = msg.bids;
  j["snapshot"] = msg.snapshot;
  if (msg.seq_id != std::nullopt) {
    j["seq_id"] = msg.seq_id.value();
  }
  if (msg.prev_seq_id != std::nullopt) {
    j["prev_seq_id"] = msg.prev_seq_id.value();
  }
}

static void to_json(nlohmann::json& j, const FundingRateMsg& msg) {
  to_json_shared(j, msg);
  j["funding_rate"] = msg.funding_rate;
  j["funding_time"] = msg.funding_time;
  if (msg.estimated_rate != std::nullopt) {
    j["estimated_rate"] = msg.estimated_rate.value();
  }
}

static void from_json_shared(const nlohmann::json& j, Message& msg) {
  j.at("exchange").get_to(msg.exchange);
  j.at("market_type").get_to(msg.market_type);
  j.at("msg_type").get_to(msg.msg_type);
  j.at("symbol").get_to(msg.symbol);
  j.at("pair").get_to(msg.pair);
  j.at("timestamp").get_to(msg.timestamp);
  j.at("json").get_to(msg.json);
}

static void from_json(const nlohmann::json& j, TradeMsg& msg) {
  from_json_shared(j, msg);
  j.at("price").get_to(msg.price);
  j.at("quantity_base").get_to(msg.quantity_base);
  j.at("quantity_quote").get_to(msg.quantity_quote);
  if (j.contains("quantity_contract")) {
    msg.quantity_contract = j.at("quantity_contract").get<double>();
  }
  j.at("side").get_to(msg.side);
  j.at("trade_id").get_to(msg.trade_id);
}

static void from_json(const nlohmann::json& j, Order& order) {
  j.at("price").get_to(order.price);
  j.at("quantity_base").get_to(order.quantity_base);
  j.at("quantity_quote").get_to(order.quantity_quote);
  if (j.contains("quantity_contract")) {
    order.quantity_contract = j.at("quantity_contract").get<double>();
  }
}

static void from_json(const nlohmann::json& j, OrderBookMsg& msg) {
  from_json_shared(j, msg);
  j.at("asks").get_to(msg.asks);
  j.at("bids").get_to(msg.bids);
  j.at("snapshot").get_to(msg.snapshot);
  if (j.contains("seq_id")) {
    msg.seq_id = j.at("seq_id").get<uint64_t>();
  }
  if (j.contains("prev_seq_id")) {
    msg.prev_seq_id = j.at("prev_seq_id").get<uint64_t>();
  }
}

static void from_json(const nlohmann::json& j, FundingRateMsg& msg) {
  from_json_shared(j, msg);
  j.at("funding_rate").get_to(msg.funding_rate);
  j.at("funding_time").get_to(msg.funding_time);
  if (j.contains("estimated_rate")) {
    msg.estimated_rate = j.at("estimated_rate").get<double>();
  }
}

static inline bool is_null_terminated_view(std::string_view sv) noexcept {
  return !sv.empty() && sv.back() == '\0';
}

std::string extract_symbol(std::string_view exchange, MarketType market_type,
                           std::string_view msg) {
  const char* exchange_c_str = nullptr;
  std::string exchange_string;
  if (is_null_terminated_view(exchange)) {
    exchange_c_str = exchange.data();
  } else {
    exchange_string = std::move(std::string(exchange));
    exchange_c_str = exchange_string.c_str();
  }

  const char* msg_c_str = nullptr;
  std::string msg_string;
  if (is_null_terminated_view(msg)) {
    msg_c_str = exchange.data();
  } else {
    msg_string = std::move(std::string(msg));
    msg_c_str = exchange_string.c_str();
  }

  const char* symbol = extract_symbol(exchange_c_str, market_type, msg_c_str);
  return std::string(symbol);
}

// NOLINTNEXTLINE
MessageType get_msg_type(std::string_view exchange, std::string_view msg) {
  const char* exchange_c_str = nullptr;
  std::string exchange_string;
  if (is_null_terminated_view(exchange)) {
    exchange_c_str = exchange.data();
  } else {
    exchange_string = std::move(std::string(exchange));
    exchange_c_str = exchange_string.c_str();
  }

  const char* msg_c_str = nullptr;
  std::string msg_string;
  if (is_null_terminated_view(msg)) {
    msg_c_str = exchange.data();
  } else {
    msg_string = std::move(std::string(msg));
    msg_c_str = msg_string.c_str();
  }

  return get_msg_type(exchange_c_str, msg_c_str);
}

std::vector<TradeMsg> parse_trade(std::string_view exchange,
                                  MarketType market_type,
                                  std::string_view msg) {
  const char* exchange_c_str = nullptr;
  std::string exchange_string;
  if (is_null_terminated_view(exchange)) {
    exchange_c_str = exchange.data();
  } else {
    exchange_string = std::move(std::string(exchange));
    exchange_c_str = exchange_string.c_str();
  }

  const char* msg_c_str = nullptr;
  std::string msg_string;
  if (is_null_terminated_view(msg)) {
    msg_c_str = exchange.data();
  } else {
    msg_string = std::move(std::string(msg));
    msg_c_str = msg_string.c_str();
  }

  const char* parsed_msg = parse_trade(exchange_c_str, market_type, msg_c_str);
  nlohmann::json j = nlohmann::json::parse(parsed_msg);
  return j.get<std::vector<TradeMsg>>();
}

std::vector<OrderBookMsg> parse_l2(std::string_view exchange,
                                   MarketType market_type, std::string_view msg,
                                   int64_t timestamp) {
  const char* exchange_c_str = nullptr;
  std::string exchange_string;
  if (is_null_terminated_view(exchange)) {
    exchange_c_str = exchange.data();
  } else {
    exchange_string = std::move(std::string(exchange));
    exchange_c_str = exchange_string.c_str();
  }

  const char* msg_c_str = nullptr;
  std::string msg_string;
  if (is_null_terminated_view(msg)) {
    msg_c_str = exchange.data();
  } else {
    msg_string = std::move(std::string(msg));
    msg_c_str = msg_string.c_str();
  }

  const char* parsed_msg =
      parse_l2(exchange_c_str, market_type, msg_c_str, timestamp);
  nlohmann::json j = nlohmann::json::parse(parsed_msg);
  return j.get<std::vector<OrderBookMsg>>();
}

std::vector<FundingRateMsg> parse_funding_rate(std::string_view exchange,
                                               MarketType market_type,
                                               std::string_view msg) {
  const char* exchange_c_str = nullptr;
  std::string exchange_string;
  if (is_null_terminated_view(exchange)) {
    exchange_c_str = exchange.data();
  } else {
    exchange_string = std::move(std::string(exchange));
    exchange_c_str = exchange_string.c_str();
  }

  const char* msg_c_str = nullptr;
  std::string msg_string;
  if (is_null_terminated_view(msg)) {
    msg_c_str = exchange.data();
  } else {
    msg_string = std::move(std::string(msg));
    msg_c_str = msg_string.c_str();
  }

  const char* parsed_msg =
      parse_funding_rate(exchange_c_str, market_type, msg_c_str);
  nlohmann::json j = nlohmann::json::parse(parsed_msg);
  return j.get<std::vector<FundingRateMsg>>();
}
