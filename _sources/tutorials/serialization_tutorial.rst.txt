Serialization Tutorial
=====================

OULY provides flexible serialization frameworks for both binary and YAML formats. This tutorial covers efficient data persistence, network protocols, and configuration management.

Serialization Overview
----------------------

OULY supports two main serialization formats:

* **Binary Serialization** - High-performance, compact format with endianness control
* **YAML Serialization** - Human-readable format for configuration and data exchange

Both formats support:
* Custom types and containers
* Nested structures and STL containers
* Endianness conversion for cross-platform compatibility
* Stream-based I/O for memory efficiency

Binary Serialization
--------------------

Binary serialization provides optimal performance for data persistence and network protocols:

.. code-block:: cpp

   #include <ouly/serializers/binary_stream.hpp>
   #include <ouly/serializers/binary_serializer.hpp>
   #include <iostream>
   #include <fstream>

   struct Player {
       std::string name;
       int level;
       float health;
       std::vector<int> inventory;
   };

   int main() {
       // Create test data
       Player player;
       player.name = "Hero";
       player.level = 25;
       player.health = 87.5f;
       player.inventory = {1, 5, 3, 8, 2};
       
       // Serialize to binary stream
       ouly::binary_stream output;
       ouly::write<std::endian::little>(output, player);
       
       std::cout << "Serialized size: " << output.size() << " bytes\n";
       
       // Deserialize from binary stream
       ouly::binary_stream input(output.data(), output.size());
       Player loaded_player;
       ouly::read<std::endian::little>(input, loaded_player);
       
       // Verify data
       std::cout << "Loaded player: " << loaded_player.name 
                 << " (Level " << loaded_player.level << ")\n";
       std::cout << "Health: " << loaded_player.health << "\n";
       std::cout << "Inventory items: " << loaded_player.inventory.size() << "\n";
       
       return 0;
   }

File I/O with Binary Serialization
-----------------------------------

Save and load data to/from files with proper error handling:

.. code-block:: cpp

   #include <ouly/serializers/binary_stream.hpp>
   #include <fstream>

   struct GameState {
       int current_level;
       float elapsed_time;
       std::vector<Player> players;
       std::unordered_map<std::string, int> statistics;
   };

   bool save_game_state(const std::string& filename, const GameState& state) {
       try {
           std::ofstream file(filename, std::ios::binary);
           if (!file) {
               std::cerr << "Failed to open file for writing: " << filename << "\n";
               return false;
           }
           
           ouly::binary_ostream stream(file);
           ouly::write<std::endian::little>(stream, state);
           
           return true;
       } catch (const std::exception& e) {
           std::cerr << "Error saving game state: " << e.what() << "\n";
           return false;
       }
   }

   bool load_game_state(const std::string& filename, GameState& state) {
       try {
           std::ifstream file(filename, std::ios::binary);
           if (!file) {
               std::cerr << "Failed to open file for reading: " << filename << "\n";
               return false;
           }
           
           ouly::binary_istream stream(file);
           ouly::read<std::endian::little>(stream, state);
           
           return true;
       } catch (const std::exception& e) {
           std::cerr << "Error loading game state: " << e.what() << "\n";
           return false;
       }
   }

   int main() {
       GameState game_state;
       game_state.current_level = 5;
       game_state.elapsed_time = 1234.5f;
       game_state.statistics["enemies_defeated"] = 127;
       game_state.statistics["items_collected"] = 45;
       
       // Save to file
       if (save_game_state("savegame.dat", game_state)) {
           std::cout << "Game state saved successfully\n";
       }
       
       // Load from file
       GameState loaded_state;
       if (load_game_state("savegame.dat", loaded_state)) {
           std::cout << "Game state loaded successfully\n";
           std::cout << "Current level: " << loaded_state.current_level << "\n";
           std::cout << "Elapsed time: " << loaded_state.elapsed_time << "s\n";
       }
       
       return 0;
   }

Custom Type Serialization
--------------------------

Define serialization for your custom types:

.. code-block:: cpp

   #include <ouly/serializers/binary_serializer.hpp>

   struct Vec3 {
       float x, y, z;
       
       Vec3() = default;
       Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
   };

   // Specialize serialization for Vec3
   namespace ouly {
       template<std::endian Endian>
       void write(binary_ostream& stream, const Vec3& vec) {
           write<Endian>(stream, vec.x);
           write<Endian>(stream, vec.y);
           write<Endian>(stream, vec.z);
       }
       
       template<std::endian Endian>
       void read(binary_istream& stream, Vec3& vec) {
           read<Endian>(stream, vec.x);
           read<Endian>(stream, vec.y);
           read<Endian>(stream, vec.z);
       }
   }

   struct Transform {
       Vec3 position;
       Vec3 rotation;
       Vec3 scale;
       
       Transform() : scale(1.0f, 1.0f, 1.0f) {}
   };

   // Transform serialization is automatic (uses Vec3 serialization)

   int main() {
       Transform transform;
       transform.position = Vec3(10.0f, 20.0f, 30.0f);
       transform.rotation = Vec3(0.0f, 45.0f, 0.0f);
       transform.scale = Vec3(2.0f, 2.0f, 2.0f);
       
       // Serialize custom types
       ouly::binary_stream stream;
       ouly::write<std::endian::little>(stream, transform);
       
       // Deserialize
       ouly::binary_stream input(stream.data(), stream.size());
       Transform loaded_transform;
       ouly::read<std::endian::little>(input, loaded_transform);
       
       std::cout << "Position: (" << loaded_transform.position.x 
                 << ", " << loaded_transform.position.y 
                 << ", " << loaded_transform.position.z << ")\n";
       
       return 0;
   }

YAML Serialization
------------------

YAML provides human-readable configuration and data exchange:

.. code-block:: cpp

   #include <ouly/serializers/yaml_serializer.hpp>
   #include <iostream>

   struct DatabaseConfig {
       std::string host;
       int port;
       std::string username;
       std::string database_name;
       bool use_ssl;
       std::vector<std::string> allowed_ips;
   };

   struct ServerConfig {
       std::string server_name;
       int max_connections;
       DatabaseConfig database;
       std::unordered_map<std::string, std::string> environment_vars;
   };

   int main() {
       // Create configuration
       ServerConfig config;
       config.server_name = "GameServer";
       config.max_connections = 1000;
       config.database.host = "localhost";
       config.database.port = 5432;
       config.database.username = "gameuser";
       config.database.database_name = "gamedb";
       config.database.use_ssl = true;
       config.database.allowed_ips = {"192.168.1.0/24", "10.0.0.0/8"};
       config.environment_vars["LOG_LEVEL"] = "INFO";
       config.environment_vars["MAX_MEMORY"] = "4GB";
       
       // Serialize to YAML string
       std::string yaml_data = ouly::yml::to_string(config);
       std::cout << "YAML Configuration:\n" << yaml_data << "\n";
       
       // Deserialize from YAML string
       ServerConfig loaded_config;
       ouly::yml::from_string(loaded_config, yaml_data);
       
       std::cout << "Loaded server: " << loaded_config.server_name << "\n";
       std::cout << "Database host: " << loaded_config.database.host << "\n";
       std::cout << "SSL enabled: " << (loaded_config.database.use_ssl ? "yes" : "no") << "\n";
       
       return 0;
   }

The output YAML would look like:

.. code-block:: yaml

   server_name: GameServer
   max_connections: 1000
   database:
     host: localhost
     port: 5432
     username: gameuser
     database_name: gamedb
     use_ssl: true
     allowed_ips:
       - 192.168.1.0/24
       - 10.0.0.0/8
   environment_vars:
     LOG_LEVEL: INFO
     MAX_MEMORY: 4GB

YAML File Operations
--------------------

Work with YAML configuration files:

.. code-block:: cpp

   #include <ouly/serializers/yaml_serializer.hpp>
   #include <fstream>

   bool save_config_yaml(const std::string& filename, const ServerConfig& config) {
       try {
           std::ofstream file(filename);
           if (!file) {
               std::cerr << "Failed to open file for writing: " << filename << "\n";
               return false;
           }
           
           std::string yaml_data = ouly::yml::to_string(config);
           file << yaml_data;
           
           return true;
       } catch (const std::exception& e) {
           std::cerr << "Error saving YAML config: " << e.what() << "\n";
           return false;
       }
   }

   bool load_config_yaml(const std::string& filename, ServerConfig& config) {
       try {
           std::ifstream file(filename);
           if (!file) {
               std::cerr << "Failed to open file for reading: " << filename << "\n";
               return false;
           }
           
           std::string yaml_content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
           
           ouly::yml::from_string(config, yaml_content);
           
           return true;
       } catch (const std::exception& e) {
           std::cerr << "Error loading YAML config: " << e.what() << "\n";
           return false;
       }
   }

   int main() {
       ServerConfig config;
       // ... initialize config ...
       
       // Save to YAML file
       if (save_config_yaml("server_config.yml", config)) {
           std::cout << "Configuration saved to server_config.yml\n";
       }
       
       // Load from YAML file
       ServerConfig loaded_config;
       if (load_config_yaml("server_config.yml", loaded_config)) {
           std::cout << "Configuration loaded from server_config.yml\n";
       }
       
       return 0;
   }

Network Serialization
---------------------

Use binary serialization for network protocols:

.. code-block:: cpp

   #include <ouly/serializers/binary_stream.hpp>
   #include <vector>
   #include <cstring>

   enum class MessageType : uint8_t {
       PLAYER_MOVE = 1,
       PLAYER_ATTACK = 2,
       GAME_STATE = 3,
       CHAT_MESSAGE = 4
   };

   struct NetworkMessage {
       MessageType type;
       uint32_t player_id;
       std::vector<uint8_t> payload;
   };

   struct PlayerMoveData {
       float x, y, z;
       float velocity_x, velocity_y, velocity_z;
       uint32_t timestamp;
   };

   class NetworkProtocol {
   public:
       static std::vector<uint8_t> serialize_player_move(uint32_t player_id, 
                                                        const PlayerMoveData& move_data) {
           ouly::binary_stream stream;
           
           // Write header
           ouly::write<std::endian::little>(stream, MessageType::PLAYER_MOVE);
           ouly::write<std::endian::little>(stream, player_id);
           
           // Write payload
           ouly::write<std::endian::little>(stream, move_data);
           
           return std::vector<uint8_t>(stream.data(), stream.data() + stream.size());
       }
       
       static bool deserialize_player_move(const std::vector<uint8_t>& data,
                                          uint32_t& player_id,
                                          PlayerMoveData& move_data) {
           try {
               ouly::binary_stream stream(data.data(), data.size());
               
               // Read header
               MessageType type;
               ouly::read<std::endian::little>(stream, type);
               if (type != MessageType::PLAYER_MOVE) {
                   return false;
               }
               
               ouly::read<std::endian::little>(stream, player_id);
               
               // Read payload
               ouly::read<std::endian::little>(stream, move_data);
               
               return true;
           } catch (const std::exception&) {
               return false;
           }
       }
   };

   int main() {
       // Simulate sending a player move message
       PlayerMoveData move_data;
       move_data.x = 100.5f;
       move_data.y = 200.3f;
       move_data.z = 50.0f;
       move_data.velocity_x = 5.0f;
       move_data.velocity_y = 0.0f;
       move_data.velocity_z = 2.5f;
       move_data.timestamp = 1234567890;
       
       // Serialize for network transmission
       auto packet = NetworkProtocol::serialize_player_move(42, move_data);
       std::cout << "Packet size: " << packet.size() << " bytes\n";
       
       // Simulate receiving the packet
       uint32_t received_player_id;
       PlayerMoveData received_move_data;
       
       if (NetworkProtocol::deserialize_player_move(packet, received_player_id, received_move_data)) {
           std::cout << "Received move from player " << received_player_id << "\n";
           std::cout << "Position: (" << received_move_data.x 
                     << ", " << received_move_data.y 
                     << ", " << received_move_data.z << ")\n";
       }
       
       return 0;
   }

Endianness Handling
-------------------

Handle cross-platform endianness for network protocols and file formats:

.. code-block:: cpp

   #include <ouly/serializers/binary_stream.hpp>
   #include <bit>

   struct NetworkHeader {
       uint32_t magic_number;
       uint16_t version;
       uint16_t message_type;
       uint32_t payload_size;
   };

   class ProtocolHandler {
   public:
       // Always use network byte order (big endian) for headers
       static std::vector<uint8_t> create_packet(uint16_t message_type, 
                                                const std::vector<uint8_t>& payload) {
           NetworkHeader header;
           header.magic_number = 0x12345678;
           header.version = 1;
           header.message_type = message_type;
           header.payload_size = static_cast<uint32_t>(payload.size());
           
           ouly::binary_stream stream;
           
           // Write header in network byte order (big endian)
           ouly::write<std::endian::big>(stream, header);
           
           // Write payload
           stream.write(payload.data(), payload.size());
           
           return std::vector<uint8_t>(stream.data(), stream.data() + stream.size());
       }
       
       static bool parse_header(const std::vector<uint8_t>& data, NetworkHeader& header) {
           if (data.size() < sizeof(NetworkHeader)) {
               return false;
           }
           
           try {
               ouly::binary_stream stream(data.data(), data.size());
               ouly::read<std::endian::big>(stream, header);
               
               // Validate magic number
               return header.magic_number == 0x12345678;
           } catch (const std::exception&) {
               return false;
           }
       }
   };

   int main() {
       // Create a packet
       std::vector<uint8_t> payload = {1, 2, 3, 4, 5};
       auto packet = ProtocolHandler::create_packet(100, payload);
       
       std::cout << "Created packet with " << packet.size() << " bytes\n";
       
       // Parse the header
       NetworkHeader header;
       if (ProtocolHandler::parse_header(packet, header)) {
           std::cout << "Valid packet:\n";
           std::cout << "  Version: " << header.version << "\n";
           std::cout << "  Message type: " << header.message_type << "\n";
           std::cout << "  Payload size: " << header.payload_size << "\n";
       }
       
       return 0;
   }

Performance Considerations
--------------------------

**Binary vs YAML Performance**

.. code-block:: cpp

   #include <chrono>

   void benchmark_serialization() {
       std::vector<Player> players(1000);
       // Initialize players...
       
       auto start_time = std::chrono::high_resolution_clock::now();
       
       // Binary serialization
       ouly::binary_stream binary_stream;
       ouly::write<std::endian::little>(binary_stream, players);
       
       auto binary_time = std::chrono::high_resolution_clock::now();
       
       // YAML serialization
       std::string yaml_data = ouly::yml::to_string(players);
       
       auto yaml_time = std::chrono::high_resolution_clock::now();
       
       auto binary_duration = std::chrono::duration_cast<std::chrono::microseconds>(
           binary_time - start_time);
       auto yaml_duration = std::chrono::duration_cast<std::chrono::microseconds>(
           yaml_time - binary_time);
       
       std::cout << "Binary serialization: " << binary_duration.count() << " μs, "
                 << binary_stream.size() << " bytes\n";
       std::cout << "YAML serialization: " << yaml_duration.count() << " μs, "
                 << yaml_data.size() << " bytes\n";
   }

**Memory Efficiency**

.. code-block:: cpp

   // Use reserve() for known data sizes
   void efficient_serialization(const std::vector<LargeObject>& objects) {
       ouly::binary_stream stream;
       
       // Estimate size to avoid reallocations
       size_t estimated_size = objects.size() * sizeof(LargeObject) * 2;  // Conservative estimate
       stream.reserve(estimated_size);
       
       ouly::write<std::endian::little>(stream, objects);
   }

   // Use streaming for large datasets
   void stream_large_dataset(const std::string& filename, 
                            const std::vector<LargeObject>& objects) {
       std::ofstream file(filename, std::ios::binary);
       ouly::binary_ostream stream(file);
       
       // Write count first
       ouly::write<std::endian::little>(stream, objects.size());
       
       // Stream objects one by one
       for (const auto& obj : objects) {
           ouly::write<std::endian::little>(stream, obj);
       }
   }

Best Practices
--------------

1. **Format Selection**
   
   * Use binary for performance-critical applications (games, networking)
   * Use YAML for configuration files and human-readable data
   * Consider compression for large binary data

2. **Endianness**
   
   * Always specify endianness explicitly
   * Use network byte order (big endian) for network protocols
   * Use little endian for most file formats (more common on modern systems)

3. **Error Handling**
   
   * Always handle serialization exceptions
   * Validate data before deserializing
   * Use versioning for backward compatibility

4. **Performance**
   
   * Reserve buffer space for known data sizes
   * Use streaming for large datasets
   * Profile serialization performance in your specific use case

5. **Security**
   
   * Validate input data bounds and ranges
   * Sanitize strings and containers
   * Use checksums for data integrity

Next Steps
----------

* Learn about :doc:`memory_management` for optimized serialization buffers
* Explore :doc:`containers_tutorial` for serializable container types
* Check :doc:`../performance/index` for serialization optimization techniques
