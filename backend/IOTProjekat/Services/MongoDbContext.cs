using IOTProjekat.Models;
using MongoDB.Driver;

namespace IOTProjekat.Services
{
    public class MongoDbContext
    {
        private readonly IMongoDatabase _database;
        public MongoDbContext(IConfiguration configuration)
        {
            var connectionString = configuration["MongoDB:ConnectionString"];
            var databaseName = configuration["MongoDB:DatabaseName"];

            var client = new MongoClient(connectionString);
            
            _database = client.GetDatabase(databaseName);
        }

        public IMongoCollection<GardenSensor> GardenSensors => _database.GetCollection<GardenSensor>("GardenSensors");
    }
}
