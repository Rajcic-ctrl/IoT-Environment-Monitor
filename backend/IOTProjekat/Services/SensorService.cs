using IOTProjekat.DTOs;
using IOTProjekat.Models;
using MongoDB.Driver;

namespace IOTProjekat.Services
{
    public class SensorService : ISensorService
    {
        private readonly MongoDbContext _context;
        public SensorService(MongoDbContext context)
        {
            _context = context;
        }
        public async Task AddData(AddSensorDataDto dto)
        {
            Console.WriteLine(dto.Temperature);
            var data = new GardenSensor
            {
                Temperature = dto.Temperature,
                AirHumidity = dto.Humidity,
                Light = dto.Light,
                Time = dto.Timestamp,
                Status = dto.Status,
            };
            await _context.GardenSensors.InsertOneAsync(data);
        }

        public async Task<List<GardenSensor>> GetAll()
        {
            var data = await _context.GardenSensors.Find(_ => true).SortByDescending(x => x.Time).ToListAsync();

            return data;
        }

        public async Task<GardenSensor> GetLatest()
        {
            var data = await _context.GardenSensors.Find(_ => true).SortByDescending(x => x.Time).FirstOrDefaultAsync();

            return data;
        }
    }
}
