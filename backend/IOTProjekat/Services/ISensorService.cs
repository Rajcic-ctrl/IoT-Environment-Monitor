using IOTProjekat.DTOs;
using IOTProjekat.Models;

namespace IOTProjekat.Services
{
    public interface ISensorService
    {
        public Task AddData(AddSensorDataDto dto);
        public Task<List<GardenSensor>> GetAll();
        public Task<GardenSensor> GetLatest();
    }
}
